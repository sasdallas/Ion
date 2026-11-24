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
#include <ion/ke/ke.h>

OBJECT_DIRECTORY *ObRootDirectory = NULL;

OS_STATUS ObpInitializeRootDirectory() {
    OBJECT_ATTRIBUTES Attributes = {
        .ObjectName = NULL,
        .Flags = OB_FLAG_KERNEL_OBJECT,
        .RootDirectory = NULL
    };

    OS_STATUS Status = ObCreateDirectory(&Attributes, (VOID*)&ObRootDirectory);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

IONAPI OS_STATUS ObCreateDirectory(OBJECT_ATTRIBUTES *Attributes, VOID **ObjectOutput) {
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
