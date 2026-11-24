/**
 * @file ionkrnl/ex/init.c
 * @brief Executive initialization code
 * 
 * Finishes the upper initialization stages of the kernel, post-scheduler initialization
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ex.h>
#include <ion/ke/ps.h>
#include <ion/ke/ob.h>
#include <ion/ke/ke.h>
#include <ion/ke/ldr.h>
#include <ion/ke/os.h>
#include <ion/ion.h>

#define INIT_OR_BUGCHECK(fn) ({ OS_STATUS Status = fn(); if (!OS_SUCCESS(Status)) { KeBugCheckWithStatus(Status, #fn " failed to initialize (Ex stage)"); }});

static OS_STATUS ExpInitializeTypes() {
    OBJECT_TYPE_ATTRIBUTES Attributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = NULL,
        .DumpProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmPagedHeap
    };

    OS_STATUS Status = ObCreateObjectType("Event", &Attributes, &ExEventObjectType);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

NO_RETURN VOID ExInitialize() {
    DbgPrint("ExInitialize()\n");

    // Object manager
    INIT_OR_BUGCHECK(ObInitialize);

    // Prepare types
    INIT_OR_BUGCHECK(ExpInitializeTypes);

    OBJECT_ATTRIBUTES EventAttributes = { .ObjectName = NULL, .Flags = OB_FLAG_KERNEL_OBJECT, .RootDirectory = NULL};
    HANDLE EventHandle;
    OS_STATUS Status = OsCreateEvent(&EventHandle, &EventAttributes, SynchronizationEvent, FALSE);
    if (!OS_SUCCESS(Status)) KeBugCheckWithStatus(Status, "OsCreateEvent failed");

    DbgPrint("EventHandle = %p\n", EventHandle);
    OsSetEvent(EventHandle, NULL);

    for (;;) {
        PsYieldThread();
    }
}
