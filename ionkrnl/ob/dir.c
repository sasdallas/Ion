/**
 * @file ionkrnl/ob/dir.c
 * @brief Ob directory manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ob.h>
#include <ion/ke/os.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>

/* Standard directories */
OBJECT_DIRECTORY *ObpRootDirectoryObject = NULL;
OBJECT_DIRECTORY *ObpObjectTypesDirectoryObject = NULL;

/* Handles to them */
HANDLE ObRootDirectory = NULL;
HANDLE ObObjectTypesDirectory = NULL;

/* Pool */
static MM_POOL *ObpEntryPool = NULL;

/**
 * This is an FNV-1a hash implementation for parsing names into the hashtable
 */
static UINT32 ObpHashName(CHAR *Name, SIZE_T NameLength) {
    UINT32 Hash = 0x811C9DC5; // FNV-1a initialization

    for (SIZE_T i = 0; i < NameLength; i++) {
        Hash ^= (UCHAR)Name[i];
        Hash *= 0x01000193;
    }

    return Hash;
}

/**
 * Links an object into the directory. Expects name to be the name of a component.
 * E.g.: ObpLinkObject(ObRootDirectory, MyDirectory, "MyDirectory") links /MyDirectory/
 * Note: Expects a reference for object already added
 */
OS_STATUS ObpLinkObject(OBJECT_DIRECTORY *Directory, VOID *Object, CHAR *Name, SIZE_T NameLength) {
    UINT32 Hash = ObpHashName(Name, NameLength);
    OBJECT_DIRECTORY_ENTRY *Entry = MmAllocatePool(ObpEntryPool);
    if (!Entry) {
        return STATUS_NO_MEMORY;
    }

    Entry->Hash = Hash;
    Entry->Object = Object;
    Entry->Link = NULL;

    KeAcquirePushLockExclusive(&Directory->Lock);
    
    // Append at the tail
    OBJECT_DIRECTORY_ENTRY *DirectoryEntry = Directory->HashTable[Hash % OB_DIR_HASH_BUCKETS];
    
    if (DirectoryEntry == NULL) {
        Directory->HashTable[Hash % OB_DIR_HASH_BUCKETS] = Entry;
    } else {
        while (DirectoryEntry->Link) {
            DirectoryEntry = DirectoryEntry->Link;
        }

        DirectoryEntry->Link = Entry;
    }

    Directory->ChildCount += 1;
    KeReleasePushLockExclusive(&Directory->Lock);

    OBJECT_HEADER *Header = OBJECT_GET_HEADER(Object);
    Header->Directory = Directory;

    return STATUS_SUCCESS;
}

/**
 * Lookup an entry from a directory
 * Returns with an extra reference on the object
 */
VOID *ObpLookupEntryDirectory(OBJECT_DIRECTORY *Directory, CHAR *ComponentName, SIZE_T NameLength) {
    UINT32 Hash = ObpHashName(ComponentName, NameLength);
    KeAcquirePushLockShared(&Directory->Lock);

    OBJECT_DIRECTORY_ENTRY *Entry = Directory->HashTable[Hash % OB_DIR_HASH_BUCKETS];

    while (Entry) {
        if (Entry->Hash == Hash) {
            // The hashes match, but do the names?
            OBJECT_HEADER *Header = OBJECT_GET_HEADER(Entry->Object);
            CHAR *EntryName = Header->ObjectName;
            SIZE_T i = 0;
            if (EntryName) {
                while (i < NameLength && EntryName[i] && EntryName[i] == ComponentName[i]) i++;
            }
            if (i == NameLength && EntryName && EntryName[i] == 0) {
                ObReferenceObject(Entry->Object);
                KeReleasePushLockShared(&Directory->Lock);
                return Entry->Object;
            }
        }

        Entry = Entry->Link;
    }

    KeReleasePushLockShared(&Directory->Lock);
    return NULL;
}

/**
 * Dump children helper
 */
static VOID ObpDumpDirectoryChildren(OBJECT_DIRECTORY *Directory, INT Depth) {
    KeAcquirePushLockShared(&Directory->Lock);
    for (INT Bucket = 0; Bucket < OB_DIR_HASH_BUCKETS; Bucket++) {
        for (OBJECT_DIRECTORY_ENTRY *Entry = Directory->HashTable[Bucket]; Entry; Entry = Entry->Link) {
            OBJECT_HEADER *Header = OBJECT_GET_HEADER(Entry->Object);

            for (INT i = 0; i < Depth; i++) DbgPrintMsg("  ");
            DbgPrintMsg(
                "%s (of type %s, %p)\n",
                     Header->ObjectName ? Header->ObjectName : "<unnamed>",
                     Header->Type ? Header->Type->Name : "<unknown>", Entry->Object
            );

            if (Header->Type == ObDirectoryObjectType) {
                ObpDumpDirectoryChildren(Entry->Object, Depth + 1);
            }
        }
    }

    KeReleasePushLockShared(&Directory->Lock);
}

/**
 * Dump object directory tree
 */
VOID ObpDumpObjectTree() {
    DbgPrintMsg("\nDirectory dump:\n");
    DbgPrintMsg("/ (Directory, %p)\n", ObpRootDirectoryObject);
    ObpDumpDirectoryChildren(ObpRootDirectoryObject, 1);
}

/**
 * Prepares /ObjectTypes/
 */
static OS_STATUS ObpPrepareObjectTypesDirectory() {
    // Create object types directory
    OBJECT_ATTRIBUTES Attributes = {
        .ObjectName = "/ObjectTypes",
        .RootDirectory = NULL,
        .Flags = 0,
    };

    OS_STATUS Status = OsCreateDirectoryObject(&ObObjectTypesDirectory, 0, &Attributes);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Load the handle...
    Status = ObReferenceObjectByHandle(ObObjectTypesDirectory, NULL, KernelMode, (VOID**)&ObpObjectTypesDirectoryObject);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // This is kinda stupid but we have to link all the previously created
    // object types into the ObjectTypes directory
extern LIST_ENTRY ObpTypeList;
    LIST_ENTRY *Entry = ObpTypeList.Flink;
    while (Entry != &ObpTypeList) {
        OBJECT_TYPE *Type = CONTAINING_RECORD(Entry, OBJECT_TYPE, TypeList);

        // Link the object into the directory
        ObReferenceObject(Type);
        Status = ObpLinkObject(ObpObjectTypesDirectoryObject, Type, Type->Name, RtlStringLength(Type->Name));
        if (!OS_SUCCESS(Status)) {
            DbgPrint("Error putting type \"%s\" in /ObjectTypes: %s (0x%08x)\n", Type->Name, RtlStatusToString(Status), Status);
        }

        Entry = Entry->Flink;
    }

    return STATUS_SUCCESS;
}

/**
 * Prepares ObRootDirectory, and initializes the directory entry pool
 */
OS_STATUS ObpInitializeRootDirectory() {
    OBJECT_ATTRIBUTES Attributes = {
        .ObjectName = "/",
        .Flags = 0,
        .RootDirectory = NULL
    };

    OS_STATUS Status = OsCreateDirectoryObject(&ObRootDirectory, 0, &Attributes);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Keep these in non-paged memory, accessing them is quite common.
    Status = MmCreatePool("ObDirEntries", MM_POOL_NONPAGED, sizeof(OBJECT_DIRECTORY_ENTRY), 0, &ObpEntryPool);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Prepare /ObjectTypes
    Status = ObpPrepareObjectTypesDirectory();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

/**
 * Create a new directory.
 * WARNING: This doesn't link the directory in until you call ObInsertObject.
 */
IONAPI OS_STATUS ObCreateDirectoryObject(OBJECT_ATTRIBUTES *Attributes, VOID **ObjectOutput) {
    OBJECT_DIRECTORY *Directory;
    OS_STATUS Status = ObCreateObject(ObDirectoryObjectType, Attributes, sizeof(OBJECT_DIRECTORY), (VOID**)&Directory);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    KeInitializePushLock(&Directory->Lock);
    Directory->ChildCount = 0;
    
    if (ObjectOutput != NULL) {
        *ObjectOutput = Directory;
    }

    return STATUS_SUCCESS;
}

/**
 * (System service) Create a directory entry
 */
IONAPI OS_STATUS OsCreateDirectoryObject(OUT HANDLE *DirectoryHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes) {
    VOID *DirectoryObject;
    OS_STATUS Status = ObCreateDirectoryObject(ObjectAttributes, &DirectoryObject);
    if (!OS_SUCCESS(Status)) return Status;

    Status = ObInsertObject(DirectoryObject, DirectoryHandle);
    if (!OS_SUCCESS(Status)) return Status;

    return STATUS_SUCCESS;
}
