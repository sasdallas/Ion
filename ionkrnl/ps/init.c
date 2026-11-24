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
THREAD *PsInitializationThread = NULL;

/* Idle process */
PROCESS *PsIdleProcess = NULL;

VOID PspDestroyThreadObject(VOID *Object);

static VOID PspIdleLoop(VOID*) {
    DbgPrint("PspIdleLoop: CPU %u is idling !\n", KeGetCpu()->LogicalId);

    for (;;) {
        KeHaltExecution();
    }
}

OS_STATUS PsInitializeIdleProcess() {
    OBJECT_ATTRIBUTES IdleAttributes;
    ObInitializeObjectAttributes(
        &IdleAttributes,
        NULL,
        OB_FLAG_KERNEL_OBJECT,
        NULL
    );

    // Create the Idle process
    OS_STATUS Status = ObCreateObject(
        PsProcessObjectType,
        &IdleAttributes,
        sizeof(PROCESS),
        (VOID**)&PsIdleProcess
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    PsInitializeProcess(PsIdleProcess, NULL, "Idle", PRIORITY_IDLE);
    PsIdleProcess->AddressSpace = MmKernelAddressSpace;

    return STATUS_SUCCESS;
}

OS_STATUS PsInitializeIdleThread() {
    // Create the Idle thread
    THREAD *IdleThread;
    OS_STATUS Status = PsCreateThread(
        PsIdleProcess,
        PspIdleLoop,
        NULL,
        &IdleThread
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    KeGetCpu()->IdleThread = IdleThread;
    return STATUS_SUCCESS;
}

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

    PsInitializeProcess(PsSystemProcess, NULL, "System Process", PRIORITY_DEFAULT);
    PsSystemProcess->AddressSpace = MmKernelAddressSpace;

    // Create the initialization thread
    Status = PsCreateThread(PsSystemProcess, NULL, NULL, &PsInitializationThread);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    KeGetCpu()->CurrentThread = PsInitializationThread;
    PsInitializationThread->ThreadState = PsThreadRunning;

    return STATUS_SUCCESS;
}

OS_STATUS PsInitialize() {
    // Create the thread object type and the process object type
    OBJECT_TYPE_ATTRIBUTES TypeAttributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = PspDestroyThreadObject,
        .DumpProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmNonpagedHeap,
    };

    OS_STATUS Status = ObCreateObjectType(
        "Thread",
        &TypeAttributes,
        &PsThreadObjectType
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    TypeAttributes.DestroyProcedure = NULL;
    Status = ObCreateObjectType(
        "Process",
        &TypeAttributes,
        &PsProcessObjectType
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Initialize rest of Ps
    Status = PsInitializeProcessManager();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}
