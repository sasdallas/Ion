/**
 * @file include/ion/ke/ob/object.h
 * @brief OBJECT_HEADER, OBJECT_TYPE, etc.
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_OB_OBJECT_H
#define _ION_KE_OB_OBJECT_H 1

#include <ion/base.h>
#include <ion/ke/mm/heap.h>
#include <ion/ke/ke/pushlock.h>

#define OB_FLAG_NEW_OBJECT          0x01
#define OB_FLAG_KERNEL_OBJECT       0x02        // Equivalent to OBJ_KERNEL_HANDLE in windows
#define OB_FLAG_PERMANENT_OBJECT    0x04
#define OB_FLAG_EXCLUSIVE_OBJECT    0x08
#define OB_FLAG_OPEN_IF             0x10

#define OB_DIR_HASH_BUCKETS         37

#define OB_PATH_SEPARATOR           '/'

typedef enum _OB_OPEN_REASON {
    ObCreateHandle = 0,
    ObOpenHandle = 1,
    ObDuplicateHandle = 2,
    ObInheritHandle = 3,
} OB_OPEN_REASON;

// Object in all of these refers to the body
typedef VOID (*OB_DUMP_PROCEDURE)(VOID *Object);
typedef OS_STATUS (*OB_OPEN_PROCEDURE)(VOID *Object, OB_OPEN_REASON OpenReason, INT HandleCount);
typedef VOID (*OB_CLOSE_PROCEDURE)(VOID *Object, INT HandleCount);
typedef VOID (*OB_DESTROY_PROCEDURE)(VOID *Object);
typedef OS_STATUS (*OB_PARSE_PROCEDURE)(VOID *ParseObject, VOID *ObjectType, CHAR **CompleteName, CHAR **RemainingName, VOID *Context, VOID **Object);

typedef struct _OBJECT_TYPE_ATTRIBUTES {
    ULONG InvalidAttributes; // flags to reject on request 
    MM_HEAP *TargetHeap; // heap to allocate new objects from.

    OB_DUMP_PROCEDURE DumpProcedure;
    OB_OPEN_PROCEDURE OpenProcedure;
    OB_CLOSE_PROCEDURE CloseProcedure;
    OB_DESTROY_PROCEDURE DestroyProcedure;
    OB_PARSE_PROCEDURE ParseProcedure;
} OBJECT_TYPE_ATTRIBUTES;

typedef struct _OBJECT_TYPE {
    LIST_ENTRY TypeList;
    const CHAR *Name;
    OBJECT_TYPE_ATTRIBUTES Info;
} OBJECT_TYPE;

typedef struct _OBJECT_DIRECTORY_ENTRY {
    struct _OBJECT_DIRECTORY_ENTRY *Link;
    VOID *Object;
    UINT32 Hash;
} OBJECT_DIRECTORY_ENTRY;

typedef struct _OBJECT_DIRECTORY {
    OBJECT_DIRECTORY_ENTRY *HashTable[OB_DIR_HASH_BUCKETS];
    INT ChildCount;
    KPUSH_LOCK Lock;
} OBJECT_DIRECTORY;

typedef struct _OBJECT_ATTRIBUTES {
    CHAR* ObjectName;
    ULONG Flags;
    HANDLE RootDirectory;
} OBJECT_ATTRIBUTES;

typedef struct _OBJECT_HEADER {
    ULONG PointerCount;
    ULONG HandleCount;

    ULONG Flags;

    OBJECT_DIRECTORY *Directory;
    HANDLE RootHandle;
    OBJECT_TYPE *Type;
    CHAR *ObjectName;

    ULONG Body[0];
} OBJECT_HEADER;

#define OBJECT_GET_HEADER(Obj) (CONTAINING_RECORD(Obj, OBJECT_HEADER, Body))
#define HEADER_GET_OBJECT(Obj) ((VOID*)((Obj)->Body))

#endif
