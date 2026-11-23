/**
 * @file ionkrnl/ke/dpc.c
 * @brief Kernel DPC manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ion.h>
#include <ion/ke/ke.h>

extern VOID KiCleanupPendingWaiters();
extern VOID PsDispatchScheduler();

/*
 * Initialize a DPC object
 * The DPC objects take a function pointer and can be queued on one CPU (that being the current)
 * If you want to work balance, perhaps consider a workqueue.
 * 
 * DPCs execute with interrupts enabled but should never sleep or do work. 
 */
IONAPI VOID KeInitializeDpc(KDPC *Dpc, DEFERRED_FUNCTION Function) {
    Dpc->Context = NULL;
    Dpc->Function = Function;
    Dpc->IsEnqueued = FALSE;
}

/*
 * Queue a DPC onto the current CPU
 * Will be executed when IRQL drops below DISPATCH_LEVEL
 */
IONAPI VOID KeEnqueueDpc(KDPC *Dpc, VOID *Context) {
    KCPU *Cpu = KeGetCpu();
    BOOLEAN Restore = KeSetInterruptState(FALSE);

    if (Dpc->IsEnqueued) {
        KeSetInterruptState(Restore);
        return;
    }

    Dpc->Context = Context;
    Dpc->IsEnqueued = TRUE;
    InsertTailList(&Cpu->DpcListHead, &Dpc->Entry);
    
    KeRequestSoftwareInterrupt(DISPATCH_LEVEL);
    KeSetInterruptState(Restore);

    if (Restore && Cpu->Irql < DISPATCH_LEVEL) {
        KiDispatchPendingSoftwareInterrupt(Cpu->Irql);
    }
}

/*
 * Handle and process pending DPCs
 * Sets current IRQL to DISPATCH_LEVEL, then enters the loop to process each
 */
VOID KiDpcInterrupt() {
    KCPU *Cpu = KeGetCpu();

    KiAcknowledgeSoftwareInterrupt(DISPATCH_LEVEL);

    Cpu->Irql = DISPATCH_LEVEL;
    KeHardwareUpdateIrql(DISPATCH_LEVEL);

    // Do some other stuff beforehand
    KiCleanupPendingWaiters();
    KeUpdateTimers();

    // Processing loop for DPCs
    LIST_ENTRY *DpcList = &Cpu->DpcListHead;
    while (!IsListEmpty(DpcList)) {
        LIST_ENTRY *DpcEntry = RemoveHeadList(DpcList);
        KDPC *Dpc = CONTAINING_RECORD(DpcEntry, KDPC, Entry);

        Dpc->IsEnqueued = FALSE;

        BOOLEAN Restore = KeSetInterruptState(TRUE);
        Dpc->Function(Dpc, Dpc->Context);
        KeSetInterruptState(Restore);
    }

    // Dispatch the scheduler
    PsDispatchScheduler();
}

/*
 * Initialize DPC queue
 */
VOID KiInitializeDpcQueue() {
    KCPU *Cpu = KeGetCpu();
    InitializeListHead(&Cpu->DpcListHead);
}
