/**
 * @file ionkrnl/ps/sched.c
 * @brief Thread scheduler
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
#include <ion/ke/ps.h>
#include <ion/ke/ob.h>

#define THIS_SCHEDULER (&KeGetCpu()->Scheduler)

// should be called once per AP
OS_STATUS PsInitializeScheduler() {
    DbgPrint("PsInitializeScheduler()\n");

    KeInitializeSpinLock(&THIS_SCHEDULER->Lock);
    RtlZeroMemory(&THIS_SCHEDULER->Realtime, sizeof(SCHED_RUNTIME_QUEUE));
    RtlZeroMemory(&THIS_SCHEDULER->Timeshared, sizeof(SCHED_TIMESHARED_QUEUE));
    RtlZeroMemory(&THIS_SCHEDULER->Idle, sizeof(SCHED_RUNTIME_QUEUE));
    
    return STATUS_SUCCESS;
}

VOID PspInitializeThreadSchedule(THREAD *Thread) {
    Thread->Schedule.RemainingQuantum = SCHED_DEFAULT_QUANTUM;
}

IONAPI VOID PsTickScheduler() {

}

IONAPI VOID PsReadyThread(THREAD *Thread) {

}

IONAPI VOID PsYieldThread() {
    KeBugCheckWithMessage("PsYieldThread NYI");
}
