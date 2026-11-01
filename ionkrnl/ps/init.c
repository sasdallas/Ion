/**
 * @file ionkrnl/ps/init.c
 * @brief Process initialization code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ke/ps.h>
#include <ion/ke/ob.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>

/* Public object types */
OBJECT_TYPE *PsThreadObjectType = NULL;
OBJECT_TYPE *PsProcessObjectType = NULL;

/* System process */
PROCESS *PsSystemProcess = NULL;

OS_STATUS PsInitializeSystemProcess() {
    OBJECT_ATTRIBUTES SystemAttributes;
    ObInitializeObjectAttributes(&SystemAttributes, NULL, OB_FLAG_KERNEL_OBJECT, NULL);

    OS_STATUS Status = ObCreateObject(
        PsProcessObjectType,
        &SystemAttributes,
        sizeof(PROCESS),
        (VOID**)&PsSystemProcess
    );

    if (!OS_SUCCESS(Status)) {
        DbgPrint("ObCreateObject() failed while creating PsSystemProcess (0x%08x)\n", Status);
        return Status;
    }

    KeInitializeSpinLock(&PsSystemProcess->Spinlock);

    // The system process begins with a PID of 0
    PsSystemProcess->Pid = 0;
    PsSystemProcess->AddressSpace = MmKernelAddressSpace;
    InitializeListHead(&PsSystemProcess->ThreadList);
    RtlCopyMemory(PsSystemProcess->ImageName, "System Process", sizeof("System Process"));

    return STATUS_SUCCESS;
}

OS_STATUS PsInitialize() {
    // Create the thread object type and the process object type
    OBJECT_TYPE_ATTRIBUTES TypeAttributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = NULL,
        .DumpProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmNonpagedHeap,
    };

    OS_STATUS Status = ObCreateObjectType(
        "PsThread",
        &TypeAttributes,
        &PsThreadObjectType
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    Status = ObCreateObjectType(
        "PsProcess",
        &TypeAttributes,
        &PsProcessObjectType
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}
