/**
 * @file ionkrnl/ps/sched.c
 * @brief Thread scheduler
 * 
 * This is a ULE based algorithm from FreeBSD
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
extern NO_RETURN VOID PspSwitchThread2(THREAD *Next);

#define KeGetScheduler() (&KeGetCpu()->Scheduler)
#define PspLockScheduler(Scheduler) ({ KIRQL Irql; KeAcquireSpinLock(&(Scheduler)->Lock, &Irql); Irql; })
#define PspUnlockScheduler(Scheduler, Irql) ({ KeReleaseSpinLock(&(Scheduler)->Lock, (Irql)); })
#define PspLockSchedulerElevated(Scheduler) ({ KeAcquireSpinLockAtDpcLevel(&(Scheduler)->Lock); })
#define PspUnlockSchedulerElevated(Scheduler) ({ KeReleaseSpinLockAtDpcLevel(&(Scheduler)->Lock); })

VOID PspFinishThreadSwitch(THREAD *Previous);
VOID PspReapThread(THREAD *Thread);

/* Scheduler is expected to be locked. */
static VOID PspInsertRealtimeQueue(SCHEDULER *Scheduler, THREAD *Thread, BOOLEAN AtHead) {
    SCHED_RUNTIME_QUEUE *Queue = &Scheduler->Realtime;
    KPRIORITY Priority = Thread->Priority;
    ASSERT(Priority >= PRIORITY_LOW_REALTIME && Priority < PRIORITY_MAX);

    LIST_ENTRY *List = &Queue->Queues[Priority];
    if (AtHead) {
        InsertHeadList(List, &Thread->Schedule.Entry);
    } else {
        InsertTailList(List, &Thread->Schedule.Entry);
    }

    Queue->Bitmap |= 1U << Priority;
    Thread->Schedule.CurrentQueue = SCHED_QUEUE_REALTIME;
}

/* Scheduler is expected to be locked. */
static THREAD *PspRemoveRealtimeQueue(SCHEDULER *Scheduler) {
    SCHED_RUNTIME_QUEUE *Queue = &Scheduler->Realtime;
    if (Queue->Bitmap == 0) return NULL;

    INT Priority = 31 - __builtin_clz(Queue->Bitmap);
    LIST_ENTRY *List = &Queue->Queues[Priority];
    LIST_ENTRY *Entry = RemoveHeadList(List);
    if (IsListEmpty(List)) {
        Queue->Bitmap &= ~(1U << Priority);
    }

    THREAD *Thread = CONTAINING_RECORD(Entry, THREAD, Schedule.Entry);
    Thread->Schedule.CurrentQueue = SCHED_QUEUE_NONE;
    return Thread;
}

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
    Thread->Schedule.CurrentQueue = SCHED_QUEUE_TIMESHARED;

    InsertTailList(
        &Queue->Queues[Index],
        &Thread->Schedule.Entry
    );

    Queue->Bitmap |= (1UL << Index);
}

/* Scheduler is expected to be locked */
FORCE_INLINE VOID PspInsertThread(SCHEDULER *Scheduler, THREAD *Thread, BOOLEAN Preempted) {
    if (Thread->Priority >= PRIORITY_LOW_REALTIME) {
        PspInsertRealtimeQueue(
            Scheduler,
            Thread,
            Preempted && Thread->Schedule.RemainingQuantum < SCHED_DEFAULT_QUANTUM
        );
    } else if (Thread->Priority == PRIORITY_IDLE) {
        ASSERT(0);
    } else {
        PspInsertTimesharedQueue(Scheduler, Thread, Preempted);
    }

    Thread->ThreadState = PsThreadReady;
}

/**
 * Charge a thread for accumulated runtime, subtracting from Scheduler->LastTime
 * Re-evaluates the remaining quantum of a thread too
 */
static VOID PspChargeRuntime(THREAD *Thread, UINT64 Now) {
    THREAD_SCHEDULE *Schedule = &Thread->Schedule;

    UINT64 Elapsed = Now - Schedule->LastTime;
    Schedule->LastTime = Now;
    Schedule->RunTime += Elapsed;

    if (Elapsed >= Schedule->RemainingQuantum) {
        Schedule->RemainingQuantum = 0;
    } else {
        Schedule->RemainingQuantum -= Elapsed;
    }
}

/**
 * PspFinishThreadSwitch runs after switching context into the next thread to run
 * It's my way of safely reinserting Previous into the CPU's scheduler queue
 */
VOID PspFinishThreadSwitch(THREAD *Previous) {
    KCPU *Cpu = KeGetCpu();
    ASSERT(Cpu->Irql == DISPATCH_LEVEL);

    // Reinsert the thread if needed
    PspLockSchedulerElevated(&Cpu->Scheduler);
    if (Previous != NULL && Previous != Cpu->IdleThread && Previous->ThreadState == PsThreadRunning) {
        PspInsertThread(&Cpu->Scheduler, Previous, TRUE);
    }
    PspUnlockSchedulerElevated(&Cpu->Scheduler);

    // Unlock the thread if its needed
    if (Previous != NULL && Previous->UnlockAfterSwitch) {
        Previous->UnlockAfterSwitch = FALSE;
        KeReleaseSpinLockAtDpcLevel(&Previous->Spinlock);
    }

    if (Previous != NULL && Previous->ThreadState == PsThreadTerminated) {
        PspReapThread(Previous);
    }

    Cpu->CurrentThread->Schedule.LastTime = HalQueryMonotonicTime();

    // Enable interrupts first so KeLowerIrql can dispatch pending DPCs
    KeSetInterruptState(TRUE);
    KeLowerIrql(Cpu->CurrentThread->ResumeIrql);
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
    THREAD *Thread = PspRemoveRealtimeQueue(Scheduler);
    if (Thread != NULL) {
        return Thread;
    }
    
    Thread = PspSelectTimesharedThread(Scheduler);
    if (Thread != NULL) {
        return Thread;
    }

    return KeGetCpu()->IdleThread;
}

/**
 * Called after the dispatcher is done with its work
 * IRQL is DISPATCH_LEVEL and IRQs are disabled
 * Handles yielding the thread
 */
VOID PsDispatchScheduler() {
    KCPU *Cpu = KeGetCpu();
    THREAD *Current = Cpu->CurrentThread;
    if (UNLIKELY(Current == NULL)) return;

    SCHEDULER *Scheduler = &Cpu->Scheduler;
    PspLockSchedulerElevated(Scheduler);

    // Update the current thread's runtime
    PspChargeRuntime(Current, HalQueryMonotonicTime());
    
    // We out of quantum?
    BOOLEAN QuantumExpired = Current->Schedule.RemainingQuantum == 0;
    if (QuantumExpired) {
        Current->Schedule.RemainingQuantum = SCHED_DEFAULT_QUANTUM;
    }

    // Determine whether we should switch threads
    // If the realtime queue has stuff, determine whether its priority wins over us
    // Otherwise, maybe there's a timeshared thread that we should beat.
    UINT32 Realtime = Scheduler->Realtime.Bitmap;
    BOOLEAN ShouldSwitch;
    if (Realtime != 0) {
        INT HighestPriority = 31 - __builtin_clz(Realtime);

        ShouldSwitch = 
            HighestPriority > Current->Priority ||
            (QuantumExpired && HighestPriority == Current->Priority);
    } else {
        ShouldSwitch =
            Scheduler->Timeshared.Bitmap != 0 &&
            (Current == Cpu->IdleThread || (QuantumExpired && Current->Priority < PRIORITY_LOW_REALTIME));
    }

    // If we need to switch, then switch.
    THREAD *Next = ShouldSwitch ? PspSelectThread(Scheduler) : NULL;
    PspUnlockSchedulerElevated(Scheduler);

    if (Next != NULL) {
        Current->ResumeIrql = DISPATCH_LEVEL;
        PspSwitchThread(Next);
        KeSetInterruptState(FALSE);
    }
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

static VOID PspTickSchedulerDpc(KDPC *Dpc, VOID *Context) {
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

VOID PspInitializeThreadSchedule(THREAD *Thread) {
    Thread->Priority = Thread->Process->Priority;
    Thread->ResumeIrql = PASSIVE_LEVEL;
    Thread->UnlockAfterSwitch = FALSE;
    Thread->Schedule.CurrentQueue = SCHED_QUEUE_NONE;
    Thread->Schedule.RemainingQuantum = SCHED_DEFAULT_QUANTUM;
    Thread->Schedule.RunTime = 0;
    Thread->Schedule.SleepTime = 0;

    Thread->Schedule.Interactive = FALSE;
    Thread->Schedule.LastTime = 0;
}

OS_STATUS PsInitializeScheduler() {
    DbgPrint("PsInitializeScheduler()\n");

    SCHEDULER *Scheduler = KeGetScheduler();
    KeInitializeSpinLock(&Scheduler->Lock);
    KeInitializeDpc(&Scheduler->Dpc, PspTickSchedulerDpc);
    RtlZeroMemory(&Scheduler->Realtime, sizeof(SCHED_RUNTIME_QUEUE));
    RtlZeroMemory(&Scheduler->Idle, sizeof(SCHED_RUNTIME_QUEUE));
    for (INT i = 0; i < SCHED_NUM_QUEUES; i++) {
        InitializeListHead(&Scheduler->Realtime.Queues[i]);
    }

    PspInitializeTimesharedQueue(Scheduler);
    
    THREAD *Thread = KeGetCurrentThread();
    if (Thread) Thread->Schedule.LastTime = HalQueryMonotonicTime();

    return STATUS_SUCCESS;
}

IONAPI VOID PsTickScheduler() {
    SCHEDULER *Scheduler = KeGetScheduler();
    KeEnqueueDpc(&Scheduler->Dpc, NULL);
}

IONAPI VOID PsReadyThread(THREAD *Thread) {
    KIRQL Irql = PsLockThread(Thread);
    SCHEDULER *Scheduler = KeGetScheduler();

    // Insert the thread's schedule
    KIRQL SchedIrql = PspLockScheduler(Scheduler);
    PspInsertThread(Scheduler, Thread, FALSE); // Updates ThreadState to PsThreadReady
    THREAD *Current = KeGetCurrentThread();

    // If this thread beats the current thread then we should yield now
    if (LIKELY(Current)) {
        if (Current == KeGetCpu()->IdleThread || (Thread->Priority >= PRIORITY_LOW_REALTIME && Thread->Priority > Current->Priority)) {
            KeRequestSoftwareInterrupt(DISPATCH_LEVEL);
        }
    }

    PspUnlockScheduler(Scheduler, SchedIrql);

    PsUnlockThread(Thread, Irql);
}

IONAPI VOID PsYieldThreadLocked() {
    KCPU *Cpu = KeGetCpu();
    SCHEDULER *Scheduler = &Cpu->Scheduler;
    THREAD *Current = Cpu->CurrentThread;
    ASSERT(KeSetInterruptState(FALSE) == TRUE);

    // Must re-enter at DISPATCH_LEVEL
    ASSERT(Cpu->Irql == DISPATCH_LEVEL);

    // Select a new thread from the scheduler
    PspLockSchedulerElevated(Scheduler);

    THREAD *Next = Cpu->NextThread;
    if (Next) {
        Cpu->NextThread = NULL;
    } else {
        Next = PspSelectThread(Scheduler);
    }

    PspUnlockSchedulerElevated(Scheduler);

    PspChargeRuntime(Current, HalQueryMonotonicTime());
    Current->Schedule.RemainingQuantum = SCHED_DEFAULT_QUANTUM;
    Current->ResumeIrql = Cpu->Irql;
    Current->UnlockAfterSwitch = TRUE;
    PspSwitchThread(Next);
}

IONAPI VOID PsYieldThread() {
    KCPU *Cpu = KeGetCpu();
    SCHEDULER *Scheduler = &Cpu->Scheduler;

    ASSERT(KeSetInterruptState(FALSE) == TRUE);

    // Go to DISPATCH_LEVEL right now
    KIRQL OldIrql;
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    // Find another thread
    PspLockSchedulerElevated(Scheduler);

    THREAD *Current = Cpu->CurrentThread;
    THREAD *Next = Cpu->NextThread;
    if (Next) {
        Cpu->NextThread = NULL;
    } else if (Current->Priority >= PRIORITY_LOW_REALTIME && (Scheduler->Realtime.Bitmap >> Current->Priority) == 0) {
        Next = Current;
    } else {
        Next = PspSelectThread(Scheduler);
    }

    PspUnlockSchedulerElevated(Scheduler);

    // If we are trying to switch to current don't bother doing anything
    if (Next == Current || Next == Cpu->IdleThread) {
        KeSetInterruptState(TRUE);
        KeLowerIrql(OldIrql);
        return;
    }

    PspChargeRuntime(Current, HalQueryMonotonicTime());
    Current->Schedule.RemainingQuantum = SCHED_DEFAULT_QUANTUM;
    Current->ResumeIrql = OldIrql;
    PspSwitchThread(Next);
}

// Don't call this, used on AP initialization to leave the current thread.
NO_RETURN VOID PsSwitchToNextThread() {
    KCPU *Cpu = KeGetCpu();
    SCHEDULER *Scheduler = &Cpu->Scheduler;

    KeSetInterruptState(FALSE);

    // Go to DISPATCH_LEVEL right now
    KIRQL OldIrql;
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    // Find another thread
    PspLockSchedulerElevated(Scheduler);

    THREAD *Current = Cpu->CurrentThread;
    THREAD *Next = Cpu->NextThread;
    if (Next) {
        Cpu->NextThread = NULL;
    } else {
        Next = PspSelectThread(Scheduler);
    }

    ASSERT(Next);
    PspUnlockSchedulerElevated(Scheduler);
    PspSwitchThread2(Next);
}
