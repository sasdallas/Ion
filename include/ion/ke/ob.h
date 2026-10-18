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

IONAPI OS_STATUS ObCreateObject(
    OBJECT_TYPE *ObjectType,
    OBJECT_ATTRIBUTES *ObjectAttrbutes,
    ULONG ObjectBodySize,
    VOID **ObjectOutput
);

IONAPI OS_STATUS ObCreateObjectType(
    const CHAR *TypeName,
    OBJECT_TYPE_ATTRIBUTES *TypeAttributes,
    OBJECT_TYPE **TypeOutput
);

IONAPI VOID ObDereferenceObject(VOID *Body);
IONAPI VOID ObReferenceObject(VOID *Body);

IONAPI OS_STATUS ObOpenObjectByPointer(
    VOID *Object,
    ULONG HandleAttributes,
    ACCESS_MASK DesiredAccess,
    OBJECT_TYPE *ObjectType,
    HANDLE **HandleOut
);

/* Ob initialization functions */
OS_STATUS ObInitializeTypeManager();

#endif
