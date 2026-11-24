/**
 * @file ionkrnl/ob/init.c
 * @brief Object manager initialization
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ob.h>

/* Directory type */
OBJECT_TYPE *ObDirectoryObjectType = NULL;

extern OS_STATUS ObpInitializeRootDirectory();

OS_STATUS ObInitialize() {
    OBJECT_TYPE_ATTRIBUTES DirectoryTypeAttributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = NULL,
        .DumpProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmNonpagedHeap,
    };

    OS_STATUS Status = ObCreateObjectType("Directory", &DirectoryTypeAttributes, &ObDirectoryObjectType);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    Status = ObpInitializeRootDirectory();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}
