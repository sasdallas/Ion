/**
 * @file ionkrnl/ob/handle.c
 * @brief Ob handle manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ob.h>
#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/ke/ps.h>

#define OB_LEAF_ENTRIES (PAGE_SIZE / sizeof(HANDLE_TABLE_ENTRY))
#define OB_BRANCH_ENTRIES (PAGE_SIZE / sizeof(VOID*))
#define OB_TABLE_LEVEL(x) ((ULONG_PTR)(x) & 3)
#define OB_TABLE_BASE(x) ((VOID*)((ULONG_PTR)(x) & ~(ULONG_PTR)3))
#define OB_MAX_TABLE_LEVEL 3

/**
 * Create a handle table for a new process.
 * Lock zero protects table growth and the freelist. The other locks protect entry buckets.
 */
static OS_STATUS ObpCreateHandleTable(HANDLE_TABLE **TableOut) {
    HANDLE_TABLE *Table;
    OS_STATUS Status = MmAllocateHeap2(MmPagedHeap, sizeof(HANDLE_TABLE), HEAP_FLAG_DEFAULT, HEAP_TAG("HTbl"), (VOID**)&Table);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Prepare pushlocks
    for (INT i = 0; i < 4; i++) KeInitializePushLock(&Table->HandleTableLock[i]);
    KeInitializePushLock(&Table->HandleContentionEvent);
    
    // Allocate the first level table structures
    HANDLE_TABLE_ENTRY *RootTbl;
    Status = MmAllocateHeap2(MmPagedHeap, PAGE_SIZE, HEAP_FLAG_DEFAULT, HEAP_TAG("HTb2"), (VOID**)&RootTbl);
    if (!OS_SUCCESS(Status)) {
        MmFreeHeap(Table);
        return Status;
    }

    HANDLE_TABLE_ENTRY *Entry = &RootTbl[0];
    
    // Setup the remaining handles
    Entry->NextFreeTableEntry = (ULONG)-1;
    Entry++;

    for (INT i = 1; i < OB_LEAF_ENTRIES; i++) {
        Entry->NextFreeTableEntry = ((i == OB_LEAF_ENTRIES-1) ? 0 : OBJ_INDEX_TO_HANDLE(i+1));
        Entry++;
    }

    // Configure freelist
    Table->FirstFreeHandle = OBJ_INDEX_TO_HANDLE(1);
    Table->LastFreeHandleEntry = &RootTbl[OB_LEAF_ENTRIES-1];
    Table->HandleCount = 0;
    Table->Table = RootTbl;
    *TableOut = Table;
    return Status;
}

/**
 * Adds another leaf to the handle table
 * Assumes HandleTableLock[0] is held (the table expansion lock)
 */
static OS_STATUS ObpExpandHandleTable(HANDLE_TABLE *Table) {
    // Calculate current table capacity
    ULONG_PTR Capacity = OB_LEAF_ENTRIES;
    for (ULONG i = 0; i < OB_TABLE_LEVEL(Table->Table); i++) {
        Capacity *= OB_BRANCH_ENTRIES;
    }

    // Do we need to grow the root?
    ULONG NextIndex = Table->HandleCount + 1;
    if (NextIndex > ((ULONG)-1 / OBJ_INDEX_TO_HANDLE(1))) {
        return STATUS_TOO_MANY_OPENED_FILES;
    }

    ULONG CurrentLevel = OB_TABLE_LEVEL(Table->Table);
    BOOLEAN GrowingRoot = NextIndex == Capacity;

    // Determine the depth (+ slot to insert at)
    ULONG Depth;
    VOID **InsertSlot = NULL;
    if (GrowingRoot == TRUE) {
        if (CurrentLevel == OB_MAX_TABLE_LEVEL) {
            return STATUS_TOO_MANY_OPENED_FILES;
        }

        // Expand to current level.
        Depth = CurrentLevel;
    } else {
        // Descend until we reach the missing link..
        ULONG Offset = NextIndex;
        VOID **Branch = OB_TABLE_BASE(Table->Table);
        Depth = CurrentLevel;
        while (Depth != 0) {
            Capacity /= OB_BRANCH_ENTRIES;
            InsertSlot = &Branch[Offset / Capacity];
            Offset %= Capacity;
            Depth--;
            if (*InsertSlot == NULL) break;
            Branch = *InsertSlot;
        }
    }

    // Allocate the needed pages
    ULONG NumPages = (Depth + 1);
    if (GrowingRoot) NumPages += 1;

    VOID *Pages[OB_MAX_TABLE_LEVEL + 1];
    for (ULONG i = 0; i < NumPages; i++) {
        OS_STATUS Status = MmAllocateHeap2(MmPagedHeap, PAGE_SIZE, HEAP_FLAG_DEFAULT, HEAP_TAG("HTb2"), &Pages[i]);
        if (!OS_SUCCESS(Status)) {
            for (ULONG j = 0; j < i; j++) {
                MmFreeHeap(Pages[j]);
            }

            return Status;
        } 
    }

    // Prepare the final leaf's freechain
    HANDLE_TABLE_ENTRY *Leaf = (HANDLE_TABLE_ENTRY*)Pages[0];
    for (INT i = 0; i < OB_LEAF_ENTRIES; i++) {
        Leaf[i].NextFreeTableEntry = ((i == OB_LEAF_ENTRIES-1) ? 0 : OBJ_INDEX_TO_HANDLE(NextIndex+i+1));
    }

    // Now, setup the branches.
    for (ULONG i = 1; i < NumPages; i++) {
        VOID **Branch = Pages[i];
        for (ULONG j = 0; j < OB_BRANCH_ENTRIES; j++) Branch[j] = NULL;
        Branch[0] = Pages[i-1];
    }

    // Finally, insert the slot
    if (GrowingRoot) {
        VOID **Branch = Pages[NumPages-1];
        Branch[0] = OB_TABLE_BASE(Table->Table);
        Branch[1] = Pages[NumPages-2];
        Table->Table = (HANDLE_TABLE_ENTRY*)((ULONG_PTR)Branch | (CurrentLevel+1));
    } else {
        *InsertSlot = Pages[NumPages-1];
    }

    Table->FirstFreeHandle = OBJ_INDEX_TO_HANDLE(NextIndex);
    Table->LastFreeHandleEntry = &Leaf[OB_LEAF_ENTRIES-1];
    return STATUS_SUCCESS;
}

/**
 * Find a handle entry
 * Descends down each branch of the table onto the leaf page.
 * Returns NULL if it couldn't find anything.
 */
static HANDLE_TABLE_ENTRY *ObpLookupHandleEntry(HANDLE_TABLE *Table, HANDLE Handle) {
    ULONG Index = OBJ_HANDLE_TO_INDEX(Handle);
    VOID **Branch = OB_TABLE_BASE(Table->Table);
    ULONG Depth = OB_TABLE_LEVEL(Table->Table);

    ULONG_PTR Capacity = OB_LEAF_ENTRIES;
    for (ULONG i = 0; i < Depth; i++) {
        Capacity *= OB_BRANCH_ENTRIES;
    }

    if (Index >= Capacity) return NULL;

    while (Depth != 0) {
        Capacity /= OB_BRANCH_ENTRIES;
        Branch = ((VOID**)Branch)[Index / Capacity];
        Index %= Capacity;

        if (Branch == NULL) {
            return NULL;
        }

        Depth--;
    }

    return &((HANDLE_TABLE_ENTRY*)Branch)[Index];
}

/**
 * Create and add a handle to a process' handle table
 */
static OS_STATUS ObpCreateHandle(HANDLE_TABLE *Table, VOID *Object, HANDLE *HandleOut) {
    while (TRUE) {
        ULONG OldHead = InterlockedLoad(&Table->FirstFreeHandle);
        if (OldHead == 0) {
            // We need to expand, which is protected under table lock 0.
            KeAcquirePushLockExclusive(&Table->HandleTableLock[0]);
            OS_STATUS Status = ObpExpandHandleTable(Table);
            KeReleasePushLockExclusive(&Table->HandleTableLock[0]);
            
            if (!OS_SUCCESS(Status)) {
                return Status;
            }

            continue;
        }

        HANDLE Handle = (HANDLE)OBJ_INDEX_TO_HANDLE(OldHead);
        HANDLE_TABLE_ENTRY *Entry = ObpLookupHandleEntry(Table, Handle);

        // The CAS here synchronizes the case when two threads observe FirstFreeHandle
        // at the same time for the same value.
        KPUSH_LOCK *Lock = &Table->HandleTableLock[OldHead % 4];
        KeAcquirePushLockShared(Lock);

        ULONG Next = Entry->NextFreeTableEntry;
        Next = InterlockedCompareExchange(&Table->FirstFreeHandle, Next, OldHead);

        KeReleasePushLockShared(Lock);

        if (Next != OldHead) {
            continue;
        }

        // We own Handle now
        InterlockedIncrement(&Table->HandleCount);
        Entry->Object = Object;
        *HandleOut = Handle;
        return STATUS_SUCCESS;
    }
}

/**
 * Initialize the object manager handle table for a given process.
 * Inherits handles from parent.
 */
OS_STATUS ObInitProcess(PROCESS *Parent, PROCESS *Process) {
    assert(Parent == NULL && "NYI for anything except System Process");
    return ObpCreateHandleTable(&Process->HandleTable);
}

/**
 * Insert an object into the current process' handle table.
 */
OS_STATUS ObInsertObject(VOID *Object, HANDLE *HandleOut) {
    PROCESS *Process = PsGetCurrentProcess();

    OBJECT_HEADER *Header = OBJECT_GET_HEADER(Object);
    OBJECT_TYPE_ATTRIBUTES *Info = &Header->Type->Info;

    // The handle owns a reference independent of the caller's reference
    ObReferenceObject(Object);
    
    // Open the object, if needed
    LONG HandleCount = InterlockedIncrement((LONG*)&Header->HandleCount);
    if (Info->OpenProcedure != NULL) {
        OS_STATUS Status = Info->OpenProcedure(Object, ObCreateHandle, HandleCount);
        if (!OS_SUCCESS(Status)) {
            InterlockedDecrement((LONG*)&Header->HandleCount);
            ObDereferenceObject(Object);
            return Status;
        }
    }

    // Create a handle to the object
    OS_STATUS Status = ObpCreateHandle(Process->HandleTable, Object, HandleOut);
    if (!OS_SUCCESS(Status)) {
        HandleCount = InterlockedDecrement((LONG*)&Header->HandleCount);
        if (Info->OpenProcedure != NULL && Info->CloseProcedure != NULL) {
            Info->CloseProcedure(Object, HandleCount);
        }

        ObDereferenceObject(Object);
    }

    return Status;
}

/**
 * Gets the object from a handle reference.
 */
IONAPI OS_STATUS ObReferenceObjectByHandle(HANDLE Handle, OBJECT_TYPE *Type, KPROCESSOR_MODE AccessMode, VOID **Object) {
    *Object = NULL;
    ULONG_PTR Index = OBJ_HANDLE_TO_INDEX(Handle);
    if (Index == 0 || Index > (ULONG)-1 / 4) {
        return STATUS_INVALID_HANDLE;
    }

    HANDLE_TABLE *Table = PsGetCurrentProcess()->HandleTable;
    DbgPrint("Table = %p\n", Table);

    KPUSH_LOCK *Lock = &Table->HandleTableLock[Index % 4];
    KeAcquirePushLockShared(Lock);
    HANDLE_TABLE_ENTRY *Entry = ObpLookupHandleEntry(Table, Handle);

    DbgPrint("Entry = %p Entry->Object = %p\n", Entry, Entry->Object);

    OS_STATUS Status = STATUS_INVALID_HANDLE;
    if (Entry && Entry->Object) {
        VOID *Body = Entry->Object;
        if (Type && OBJECT_GET_HEADER(Body)->Type != Type) {
            Status = STATUS_INVALID_HANDLE;
        } else {
            ObReferenceObject(Body);
            *Object = Body;
            Status = STATUS_SUCCESS;
        }
    }

    KeReleasePushLockShared(Lock);

    return Status;
}

/**
 * Close a handle
 */
IONAPI OS_STATUS ObCloseHandle(HANDLE Handle) {
    assert(0 && "ObCloseHandle NYI");
}

/**
 * Open an object by a pointer. This creates and adds a new handle for the object.
 */
IONAPI OS_STATUS ObOpenObjectByPointer(
    VOID *Object,
    ULONG HandleAttributes,
    ACCESS_MASK DesiredAccess,
    OBJECT_TYPE *ObjectType,
    HANDLE **HandleOut
)
{
    assert(0 && "ObOpenObjectByPointer NYI");
}
