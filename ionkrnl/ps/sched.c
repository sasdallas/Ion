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
#include <ion/hal.h>

extern VOID PspSwitchThread(THREAD *Next);

#define KeGetScheduler() (&KeGetCpu()->Scheduler)
#define PspLockScheduler(Scheduler) ({ KIRQL Irql; KeAcquireSpinLock(&(Scheduler)->Lock, &Irql); Irql; })
#define PspUnlockScheduler(Scheduler, Irql) ({ KeReleaseSpinLock(&(Scheduler)->Lock, (Irql)); })
#define PspLockSchedulerElevated(Scheduler) ({ KeAcquireSpinLockAtDpcLevel(&(Scheduler)->Lock); })
#define PspUnlockSchedulerElevated(Scheduler) ({ KeReleaseSpinLockAtDpcLevel(&(Scheduler)->Lock); })
#define PspLockThread(Thread) ({ KIRQL Irql; KeAcquireSpinLock(&(Thread)->Spinlock, &Irql); Irql; })
#define PspUnlockThread(Thread, Irql) ({ KeReleaseSpinLock(&(Thread)->Spinlock, (Irql)); })

VOID PspFinishThreadSwitch(THREAD *Previous);

/* Scheduler is expected to be locked */
static VOID PspInsertTimesharedQueue(SCHEDULER *Scheduler,THREAD *Thread, BOOLEAN Preempted) {
    SCHED_CALENDAR_QUEUE *Queue = &Scheduler->Timeshared;
    UCHAR Index = Queue->InsertionIndex;

    if (Preempted == FALSE) {
        // Higher priorities mean shorter times to insertion
        Index = (Index + SCHED_TIMESHARED_DISTANCE - Thread->Priority) & (SCHED_NUM_QUEUES-1);

        // Avoid placing a therad directly at the removal index
        if (Queue->RemovalIndex != Queue->InsertionIndex
            && Index == Queue->RemovalIndex
        ) {
            Index = (Index - 1) & (SCHED_NUM_QUEUES-1);
        }
    }

    Thread->Schedule.CalendarIndex = Index;

    InsertTailList(
        &Queue->Queues[Index],
        &Thread->Schedule.Entry
    );

    Queue->Bitmap |= (1UL << Index);
}

/* Scheduler is expected to be locked */
FORCE_INLINE VOID PspInsertThread(SCHEDULER *Scheduler, THREAD *Thread, BOOLEAN Preempted) {
    if (Thread->Priority >= PRIORITY_LOW_REALTIME || Thread->Schedule.Interactive == TRUE) {
        ASSERT(0);
    } else if (Thread->Priority == PRIORITY_IDLE) {
        ASSERT(0);
    } else {
        PspInsertTimesharedQueue(Scheduler, Thread, Preempted);
    }

    Thread->ThreadState = PsThreadReady;
}

/**
 * PspFinishThreadSwitch runs after switching context into the next thread to run
 * It's my way of safely reinserting Previous into the CPU's scheduler queue
 */
VOID PspFinishThreadSwitch(THREAD *Previous) {
    KCPU *Cpu = KeGetCpu();

    KIRQL Irql = PspLockScheduler(&Cpu->Scheduler);

    if (Previous != NULL && Previous != Cpu->IdleThread && Previous->ThreadState == PsThreadRunning) {
        PspInsertThread(&Cpu->Scheduler, Previous, TRUE);
    }

    PspUnlockScheduler(&Cpu->Scheduler, Irql);
    KeSetInterruptState(TRUE);
}

/* Scheduler is expected to be locked */
static THREAD *PspSelectTimesharedThread(SCHEDULER *Scheduler) {
    SCHED_CALENDAR_QUEUE *Queue = &Scheduler->Timeshared;
    if (Queue->Bitmap == 0) return NULL;

    // Rotate the bitmap and get the removal index
    // TODO __builtin_ctzl is not portable, and a bit-scan LUT would be better.
    ULONG Start = Queue->RemovalIndex;
    UINT Rotated = (Queue->Bitmap >> Start) | (Queue->Bitmap << ((32 - Start) & 31));
    INT FirstSet = (Start + __builtin_ctzl(Rotated)) & 31;

    LIST_ENTRY *List = &Queue->Queues[FirstSet];
    ASSERT(!IsListEmpty(List));

    LIST_ENTRY *Entry = RemoveHeadList(List);
    THREAD *Thread = CONTAINING_RECORD(Entry, THREAD, Schedule.Entry);
    Thread->Schedule.CurrentQueue = SCHED_QUEUE_NONE;

    if (IsListEmpty(List)) {
        Queue->Bitmap &= ~(1UL << FirstSet);
    
        if (Queue->RemovalIndex != Queue->InsertionIndex) {
            Queue->RemovalIndex = (FirstSet + 1) % SCHED_NUM_QUEUES;
        }
    }

    return Thread;
}

FORCE_INLINE THREAD *PspSelectThread(SCHEDULER *Scheduler) {
    THREAD *Thread = PspSelectTimesharedThread(Scheduler);
    if (Thread != NULL) {
        return Thread;
    }

    return KeGetCpu()->IdleThread;
}

static VOID PspInitializeTimesharedQueue(SCHEDULER *Scheduler) {
    SCHED_CALENDAR_QUEUE *Queue = &Scheduler->Timeshared;
    Queue->Bitmap = 0UL;
    Queue->InsertionIndex = 0;
    Queue->RemovalIndex = 0;
    for (INT i = 0; i < SCHED_NUM_QUEUES; i++) {
        InitializeListHead(&Queue->Queues[i]);
    }
}

VOID PspInitializeThreadSchedule(THREAD *Thread) {
    Thread->Priority = Thread->Process->Priority;
    Thread->Schedule.CurrentQueue = SCHED_QUEUE_TIMESHARED;
    Thread->Schedule.RemainingQuantum = SCHED_DEFAULT_QUANTUM;
    Thread->Schedule.RunTime = 0;
    Thread->Schedule.SleepTime = 0;

    // LastTime is updated when the thread is picked for execution at the first time
    Thread->Schedule.LastTime = 0;
}

OS_STATUS PsInitializeScheduler() {
    DbgPrint("PsInitializeScheduler()\n");

    SCHEDULER *Scheduler = KeGetScheduler();
    KeInitializeSpinLock(&Scheduler->Lock);
    RtlZeroMemory(&Scheduler->Realtime, sizeof(SCHED_RUNTIME_QUEUE));
    RtlZeroMemory(&Scheduler->Idle, sizeof(SCHED_RUNTIME_QUEUE));

    PspInitializeTimesharedQueue(Scheduler);

    return STATUS_SUCCESS;
}

IONAPI VOID PsTickScheduler() {
    SCHEDULER *Scheduler = KeGetScheduler();
    PspLockSchedulerElevated(Scheduler);

    SCHED_CALENDAR_QUEUE *Queue = &Scheduler->Timeshared;
    if (Queue->InsertionIndex == Queue->RemovalIndex) {
        Queue->InsertionIndex = (Queue->InsertionIndex + 1) % SCHED_NUM_QUEUES;
        if ((Queue->Bitmap & (1UL << Queue->RemovalIndex)) == 0) {
            Queue->RemovalIndex = Queue->InsertionIndex;
        }
    }

    PspUnlockSchedulerElevated(Scheduler);
}

IONAPI VOID PsReadyThread(THREAD *Thread) {
    KIRQL Irql = PspLockThread(Thread);
    SCHEDULER *Scheduler = KeGetScheduler();

    // Insert the thread's schedule
    KIRQL SchedIrql = PspLockScheduler(Scheduler);
    PspInsertThread(Scheduler, Thread, FALSE); // Updates ThreadState to PsThreadReady
    PspUnlockScheduler(Scheduler, SchedIrql);

    PspUnlockThread(Thread, Irql);
}

IONAPI VOID PsYieldThread() {
    KCPU *Cpu = KeGetCpu();
    SCHEDULER *Scheduler = &Cpu->Scheduler;

    ASSERT(KeSetInterruptState(FALSE) == TRUE);

    KIRQL Irql = PspLockScheduler(Scheduler);

    THREAD *Current = Cpu->CurrentThread;
    THREAD *Next = Cpu->NextThread;
    if (Next) {
        Cpu->NextThread = NULL;
    } else {
        Next = PspSelectThread(Scheduler);
    }

    // If there's no other thread to switch to, might as well yield back.
    if (Next == NULL || Next == Current) {
        PspUnlockScheduler(Scheduler, Irql);
        KeSetInterruptState(TRUE);
        return;
    }

    // Scheduler can be unlocked here since Current is floating right now
    PspUnlockScheduler(Scheduler, Irql);
    PspSwitchThread(Next);
}
