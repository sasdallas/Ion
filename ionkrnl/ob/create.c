/**
 * @file ionkrnl/ob/create.c
 * @brief ObCreateObject family
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ob.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>

static OS_STATUS ObpAllocateObject(
    OBJECT_ATTRIBUTES *Attributes,
    OBJECT_TYPE *Type,
    ULONG ObjectBodySize,
    OBJECT_HEADER **HeaderOutput
)
{
    MM_HEAP *Heap = Type->Info.TargetHeap;

    // when name support is added it probably needs to be tacked on here
    SIZE_T AllocationSize = ObjectBodySize + sizeof(OBJECT_HEADER);
    OBJECT_HEADER *Header = MmAllocateHeap(Heap, AllocationSize);
    
    if (Header == NULL) {
        return STATUS_NO_MEMORY;
    }

    *HeaderOutput = Header;
    return STATUS_SUCCESS;
}

IONAPI OS_STATUS ObCreateObject(
    OBJECT_TYPE *ObjectType,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    ULONG ObjectBodySize,
    VOID **ObjectOutput
)
{
    if (ObjectAttributes->ObjectName != NULL) {
        KeBugCheckWithMessage("ObjectName is NYI");
    }

    if (ObjectAttributes->Flags & ObjectType->Info.InvalidAttributes) {
        return STATUS_INVALID_PARAMETER;
    }

    // Create and allocate the object
    OBJECT_HEADER *Header;
    OS_STATUS Status = ObpAllocateObject(
        ObjectAttributes,
        ObjectType,
        ObjectBodySize,
        &Header
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Prepare header
    Header->PointerCount = 1;
    Header->HandleCount = 0;
    Header->ObjectName = NULL;
    Header->Directory = NULL;
    Header->Type = ObjectType;
    Header->Flags = ObjectAttributes->Flags;
    
    *ObjectOutput = HEADER_GET_OBJECT(Header);
    return STATUS_SUCCESS;
}
