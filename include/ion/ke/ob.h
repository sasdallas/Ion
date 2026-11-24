/**
 * @file include/ion/ke/ob.h
 * @brief Object Manager
 * 
 * HEAVILY inspired by Windows NT's object manager
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_OB_H
#define _ION_KE_OB_H 1

#include <ion/base.h>
#include <ion/ke/ob/object.h>
#include <ion/ke/ob/handle.h>
#include <ion/access.h>

FORCE_INLINE VOID ObInitializeObjectAttributes(
    OBJECT_ATTRIBUTES *Attributes,
    CHAR *Name,
    ULONG Flags,
    HANDLE RootHandle
) {
    Attributes->Flags = Flags;
    Attributes->ObjectName = Name;
    Attributes->RootDirectory = RootHandle; 
}

/* Creation */
IONAPI OS_STATUS ObCreateObject(
    OBJECT_TYPE *ObjectType,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    ULONG ObjectBodySize,
    VOID **ObjectOutput
);

IONAPI OS_STATUS ObCreateObjectType(
    const CHAR *TypeName,
    OBJECT_TYPE_ATTRIBUTES *TypeAttributes,
    OBJECT_TYPE **TypeOutput
);

/* Referencing */
IONAPI VOID ObDereferenceObject(VOID *Body);
IONAPI VOID ObReferenceObject(VOID *Body);

/* Handle table */
OS_STATUS ObInitProcess(PROCESS *Parent, PROCESS *Process);
OS_STATUS ObInsertObject(VOID *Object, HANDLE *HandleOut);
IONAPI OS_STATUS ObReferenceObjectByHandle(HANDLE Handle, OBJECT_TYPE *Type, KPROCESSOR_MODE AccessMode, VOID **Object);
IONAPI OS_STATUS ObCloseHandle(HANDLE Handle);
IONAPI OS_STATUS ObOpenObjectByPointer(
    VOID *Object,
    ULONG HandleAttributes,
    ACCESS_MASK DesiredAccess,
    OBJECT_TYPE *ObjectType,
    HANDLE **HandleOut
);

/* Directory */
IONAPI OS_STATUS ObCreateDirectoryObject(OBJECT_ATTRIBUTES *Attributes, VOID **ObjectOutput);

/* Ob initialization functions */
OS_STATUS ObInitializeTypeManager();
OS_STATUS ObInitialize();

/* Private */
OS_STATUS ObpLookupObjectName(
    HANDLE RootHandle,
    CHAR *ObjectString,
    ULONG Attributes,
    OBJECT_TYPE *ObjectType,
    VOID *ParseContext,
    VOID *InsertObject,
    VOID **FoundObject
);


OS_STATUS ObpLinkObject(OBJECT_DIRECTORY *Directory, VOID *Object, CHAR *Name, SIZE_T NameLength);
VOID *ObpLookupEntryDirectory(OBJECT_DIRECTORY *Directory, CHAR *ComponentName, SIZE_T NameLength);

/* Ob types */
extern OBJECT_TYPE *ObDirectoryObjectType;
extern OBJECT_DIRECTORY *ObpRootDirectoryObject;
extern OBJECT_DIRECTORY *ObpObjectTypesDirectoryObject;
extern HANDLE ObRootDirectory, ObObjectTypesDirectory;

#endif
