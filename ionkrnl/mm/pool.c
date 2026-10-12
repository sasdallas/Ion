/**
 * @file ionkrnl/mm/pool.c
 * @brief Pool system
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/ion.h>

#define POOL_DEBUG 1
#ifdef POOL_DEBUG
#define DBG(...) DbgPrint(__VA_ARGS__)
#define POOL_ASSERT assert
#else
#define DBG(...) ((void)0)
#define POOL_ASSERT(x) ((void)0)
#endif

/* Primary pool used for allocating other pools */
static MM_POOL *MmPoolForPools = NULL;

static VOID MmInitializePool(MM_POOL *Pool, const CHAR* Name, MM_POOL_TYPE Type, SIZE_T ObjectSize, SIZE_T ObjectAlignment) {
    if (ObjectAlignment == 0) ObjectAlignment = 1; 

    KeInitializeSpinLock(&Pool->Lock);
    InitializeListHead(&Pool->Free);
    InitializeListHead(&Pool->Partial);
    InitializeListHead(&Pool->Full);

    Pool->Name = Name;
    Pool->Type = Type;
    Pool->ObjectSize = ObjectSize;
    Pool->ObjectAlignment = ObjectAlignment;
    Pool->FreeSlabCount = 0;

    // ObjectAlignment must be a power of 2
    POOL_ASSERT(
        ObjectAlignment != 0 &&
        (ObjectAlignment & (ObjectAlignment - 1)) == 0
    );

    // Calculate slab size
    SIZE_T Alignment = MAX(ObjectAlignment, ALIGNOF(VOID*));
    Pool->ObjectRealSize = ALIGN_UP(
        MAX(ObjectSize, sizeof(VOID*)),
        Alignment
    );

    Pool->ObjectOffset = ALIGN_UP(
        sizeof(MM_POOL_SLAB),
        Alignment
    );

    Pool->SlabSize = PAGE_SIZE;
    Pool->ObjectsPerSlab = (Pool->SlabSize - Pool->ObjectOffset) / Pool->ObjectRealSize;

    // TODO?
    if (Pool->ObjectOffset >= Pool->SlabSize) {
        KeBugCheckWithMessage("MmCreatePool cannot use an object of size %lu as it would be bigger than a page", ObjectSize);
    }
}

static MM_POOL_SLAB *MmCreateSlabForPool(MM_POOL *Pool) {
    MM_POOL_SLAB *Slab = MmAllocateVirtualPagesArena(
        MmKernelArena,
        NULL,
        (Pool->SlabSize / PAGE_SIZE),
        MM_ALLOC_UNINIT, // TODO Optimize allocatepool by zeroing slabs and taking zeroed physical memory
        MM_PROT_READ|MM_PROT_WRITE
    );

    if (Slab == NULL) {
        return NULL;
    }

    InitializeListHead(&Slab->Entry);
    Slab->FreeList = NULL;
    Slab->FreeCount = Pool->ObjectsPerSlab;

    // Link up the freelist
    UINT_PTR Base = (UINT_PTR)Slab + Pool->ObjectOffset;
    for (INT i = 0; i < Pool->ObjectsPerSlab; i++) {
        VOID** Next = (VOID**)(Base + i * Pool->ObjectRealSize);

        *Next = Slab->FreeList;
        Slab->FreeList = Next;
    }

    return Slab;
}

static VOID MmFreeSlab(MM_POOL *Pool, MM_POOL_SLAB *Slab) {
    MmFreeVirtualPagesArena(MmKernelArena, Slab,  (Pool->SlabSize / PAGE_SIZE));
}


OS_STATUS MmAllocatePool2(MM_POOL *Pool, UINT AllocFlags, VOID **Output) {
    KIRQL Irql;
    KeAcquireSpinLock(&Pool->Lock, &Irql);

    // Try to take a slab from partially filled
    MM_POOL_SLAB *Slab = NULL;

    if (!IsListEmpty(&Pool->Partial)) {
        Slab = CONTAINING_RECORD(RemoveHeadList(&Pool->Partial), MM_POOL_SLAB, Entry);
    } else if (!IsListEmpty(&Pool->Free)) {
        Slab = CONTAINING_RECORD(RemoveHeadList(&Pool->Free), MM_POOL_SLAB, Entry);
        Pool->FreeSlabCount -= 1;
    } else {
        // A slab must be allocated and we can't hold the lock during it
        KeReleaseSpinLock(&Pool->Lock, Irql);

        // TODO: This can cause extreme fragmentation in the slabs, will be fixed when magazines are added.
        Slab = MmCreateSlabForPool(Pool);
        if (Slab == NULL) {
            return STATUS_NO_MEMORY;
        }

        KeAcquireSpinLock(&Pool->Lock, &Irql);
    }

    // Take an object
    POOL_ASSERT(Slab->FreeCount > 0);
    VOID *Object = Slab->FreeList;
    Slab->FreeList = *(VOID**)Object;
    Slab->FreeCount -= 1;

    // Reinsert
    if (Slab->FreeCount == 0) {
        InsertHeadList(&Pool->Full, &Slab->Entry);
    } else {
        InsertHeadList(&Pool->Partial, &Slab->Entry);
    }

    KeReleaseSpinLock(&Pool->Lock, Irql);

    if ((AllocFlags & POOL_FLAG_UNINIT) == 0) {
        RtlZeroMemory(Object, Pool->ObjectSize);
    }

    *Output = Object;
    return STATUS_SUCCESS;
}

OS_STATUS MmFreePool(MM_POOL *Pool, VOID *Allocation) {
    KIRQL Irql;
    KeAcquireSpinLock(&Pool->Lock, &Irql);

    MM_POOL_SLAB *Slab = (MM_POOL_SLAB*)MmPageAlignDown((UINT_PTR)Allocation);

    // Return object to freelist
    *(VOID**)Allocation = Slab->FreeList;
    Slab->FreeList = (VOID**)Allocation;
    Slab->FreeCount += 1;

    // Maybe move it from lists
    MM_POOL_SLAB *ToFree = NULL;
    if (Slab->FreeCount == Pool->ObjectsPerSlab) {
        // Remove it from the partial list
        RemoveEntryList(&Slab->Entry);

        // Fully empty list, do we bother returning it?
        if (Pool->FreeSlabCount >= POOL_FREE_THRESHOLD) {
            ToFree = CONTAINING_RECORD(RemoveHeadList(&Pool->Free), MM_POOL_SLAB, Entry);
            Pool->FreeSlabCount -= 1;
        }

        Pool->FreeSlabCount += 1;
        InsertHeadList(&Pool->Free, &Slab->Entry);
    } else if (Slab->FreeCount == 1) {
        // We got removed from the full list and moved to the partial list
        RemoveEntryList(&Slab->Entry);
        InsertHeadList(&Pool->Partial, &Slab->Entry);
    }

    KeReleaseSpinLock(&Pool->Lock, Irql);

    if (ToFree != NULL) {
        MmFreeSlab(Pool, ToFree);
    }

    return STATUS_SUCCESS;
}

OS_STATUS MmCreatePool(
    const CHAR* Name,
    MM_POOL_TYPE Type,
    SIZE_T ObjectSize,
    SIZE_T ObjectAlignment,
    MM_POOL **Output
)
{
    MM_POOL *Pool = MmAllocatePool(MmPoolForPools);
    if (Pool == NULL) {
        return STATUS_NO_MEMORY;
    }

    MmInitializePool(Pool, Name, Type, ObjectSize, ObjectAlignment);

    *Output = Pool;
    return STATUS_SUCCESS;
}


OS_STATUS MmDestroyPool(MM_POOL *Heap) {
    KeBugCheckWithMessage("MmDestroyPool NYI");
}

OS_STATUS MiInitializePoolManager() {
    // The memory to back the MmPoolForPools must come from the VMmgr since
    // there is no other way to allocate!
    MmPoolForPools = MmAllocateVirtualPagesArena(MmKernelArena, NULL, 1, 0, MM_PROT_READ | MM_PROT_WRITE);
    ASSERT(MmPoolForPools);

    // Initialize
    MmInitializePool(MmPoolForPools, "PoolForPools", MM_POOL_NONPAGED, sizeof(MM_POOL), POOL_ALIGN_NONE);

    return STATUS_SUCCESS;
}
