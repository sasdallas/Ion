/**
 * @file ionkrnl/ob/type.c
 * @brief Ob type manager
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
#include <ion/ion.h>

/* Primary type list */
static LIST_ENTRY ObpTypeList;
static KSPIN_LOCK ObpTypeListLock;

/* Pool */
static MM_POOL *ObpTypePool;

OS_STATUS ObInitializeTypeManager() {
    InitializeListHead(&ObpTypeList);
    KeInitializeSpinLock(&ObpTypeListLock);

    OS_STATUS Status = MmCreatePool("ObType", MM_POOL_NONPAGED, sizeof(OBJECT_TYPE), POOL_ALIGN_NONE, &ObpTypePool);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }
    
    return STATUS_SUCCESS;
}

IONAPI OS_STATUS ObCreateObjectType(
    const CHAR *TypeName,
    OBJECT_TYPE_ATTRIBUTES *TypeAttributes,
    OBJECT_TYPE **TypeOutput
)
{
    OBJECT_TYPE *Type = MmAllocatePool(ObpTypePool);
    if (Type == NULL) {
        return STATUS_NO_MEMORY;
    }

    Type->Name = TypeName;
    RtlCopyMemory(&Type->Info, TypeAttributes, sizeof(OBJECT_TYPE_ATTRIBUTES));
    
    KIRQL Irql;
    KeAcquireSpinLock(&ObpTypeListLock, &Irql);
    InsertTailList(&ObpTypeList, &Type->TypeList);
    KeReleaseSpinLock(&ObpTypeListLock, Irql);

    if (TypeOutput) {
        *TypeOutput = Type;
    }

    return STATUS_SUCCESS;
}
