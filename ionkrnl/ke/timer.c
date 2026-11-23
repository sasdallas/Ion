/**
 * @file ionkrnl/ke/timer.c
 * @brief KTIMER manager
 *
 * Inspired by the pairing heap in the SO-NG Trickster kernel.
 * https://codeberg.org/SO-NG/Trickster/src/branch/main/src/common/Ds/PairingHeap.c
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/hal.h>

/**
 * Meld operation to merge two pairing heaps together, with the smaller being on top
 */
static KTIMER *KiPairingHeapMeld(KTIMER *First, KTIMER *Second) {
    if (First == NULL) return Second;
    if (Second == NULL) return First;

    if (Second->DueTime < First->DueTime) {
        KTIMER *Swap = First;
        First = Second;
        Second = Swap;
    }

    InsertHeadList(&First->Children, &Second->Entry);
    return First;
}

/**
 * Take the head of a (sub)heap
 */
static KTIMER *KiPairingHeapTakeFirst(LIST_ENTRY *List) {
    if (IsListEmpty(List)) return NULL;

    LIST_ENTRY *Entry = RemoveHeadList(List);
    return CONTAINING_RECORD(Entry, KTIMER, Entry);
}

/**
 * Pairing-heap merge-children operation (see Wikipedia page)
 */
static KTIMER *KiPairingHeapMergeChildren(KTIMER *Timer) {
    LIST_ENTRY Pairs;
    InitializeListHead(&Pairs);

    while (!IsListEmpty(&Timer->Children)) {
        KTIMER *First = KiPairingHeapTakeFirst(&Timer->Children);
        KTIMER *Second = KiPairingHeapTakeFirst(&Timer->Children);
        KTIMER *Pair = KiPairingHeapMeld(First, Second);
        InsertHeadList(&Pairs, &Pair->Entry);
    }

    KTIMER *Root = NULL;
    while (!IsListEmpty(&Pairs)) {
        Root = KiPairingHeapMeld(KiPairingHeapTakeFirst(&Pairs), Root);
    }

    return Root;
}

/**
 * Insert a timer on the CPU's pairing heap
 */
FORCE_INLINE VOID KiInsertTimer(KCPU *Cpu, KTIMER *Timer) {
    Cpu->TimerHeap = KiPairingHeapMeld(Cpu->TimerHeap, Timer);
    Timer->IsQueued = TRUE;
}

/**
 * Remove a timer from the pairing heap of its CPU
 * Expects CPU heap locked
 */
static VOID KiRemoveTimer(KCPU *Cpu, KTIMER *Timer) {
    if (Timer == Cpu->TimerHeap) {
        Cpu->TimerHeap = KiPairingHeapMergeChildren(Timer);
    } else {
        RemoveEntryList(&Timer->Entry);
        InitializeListHead(&Timer->Entry);

        KTIMER *SubHeap = KiPairingHeapMergeChildren(Timer);
        Cpu->TimerHeap = KiPairingHeapMeld(Cpu->TimerHeap, SubHeap);
    }

    Timer->IsQueued = FALSE;
}

/**
 * Timer update callback, handled through a DPC for proper signalling
 * Just walks the pairing heap to find already expired timers.
 * Has a lot of room for improvement.
 */
VOID KeUpdateTimers() {
    UINT64 Timestamp = HalQueryMonotonicTime();

    KCPU *Cpu = KeGetCpu();
    KeAcquireSpinLockAtDpcLevel(&Cpu->TimerHeapLock);

    while (Cpu->TimerHeap != NULL && Cpu->TimerHeap->DueTime < Timestamp) {
        KTIMER *Timer = Cpu->TimerHeap;
        KiRemoveTimer(Cpu, Timer);

        DISPATCH_HEADER *Header = &Timer->Header;
        PsLockDispatcherElevated(Header);
        if (KeProcessObjectWaiters(Header, FALSE, STATUS_SUCCESS) == 0) {
            Header->SignalCount = 1;
        }
        PsUnlockDispatcherElevated(Header);

        // Enqueue the DPC if required
        if (Timer->Dpc) {
            KeEnqueueDpc(Timer->Dpc, NULL);
        }

        // Re-insert the timer if it has a period
        if (Timer->Period != 0) {
            Timer->DueTime = HalQueryMonotonicTime() + Timer->Period;
            KiInsertTimer(Cpu, Timer);
        }
    }

    KeReleaseSpinLockAtDpcLevel(&Cpu->TimerHeapLock);
}

/**
 * Initialize a timer structure
 */
IONAPI VOID KeInitializeTimerEx(KTIMER *Timer, KTIMER_TYPE Type) {
    Timer->Dpc = NULL;
    InitializeListHead(&Timer->Children);
    InitializeListHead(&Timer->Entry);
    Timer->DueTime = 0;
    Timer->Period = 0;
    Timer->IsQueued = FALSE;
    PsInitializeDispatcher(&Timer->Header, Type == NotificationTimer ? SatisfyNone : SatisfyReset);
}

/**
 * Set a timer, scheduling it for the future.
 * Negative DueTime produces a relative timer while positive DueTime is an absolute timer.
 * 
 * Note that for Period, the timer will be reset but its internal state will still be left as signalled
 * unless you are using a SynchronizationTimer
 */
IONAPI BOOLEAN KeSetTimerEx(KTIMER *Timer, LONGLONG DueTime, LONG Period, KDPC *Dpc) {
    // We have to raise IRQL to DISPATCH_LEVEL before we do anything so we can
    // get exclusive access to the juicy pairing heap
    KIRQL Irql;
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);

    // Now we can lock the CPU's stuff
    KCPU *Cpu = KeGetCpu();
    KeAcquireSpinLockAtDpcLevel(&Cpu->TimerHeapLock);

    // Dequeue the timer if it was queued originally
    BOOLEAN WasQueued = FALSE;
    if (Timer->IsQueued) {
        KiRemoveTimer(Cpu, Timer);
        WasQueued = TRUE;
    }

    // Calculate due time first
    if (DueTime < 0) {
        // Negative due time means this is system relative
        UINT64 HalTime = HalQueryMonotonicTime();
        Timer->DueTime = HalTime + (-DueTime);
    } else {
        Timer->DueTime = DueTime;
    }

    Timer->Period = Period;
    Timer->Dpc = Dpc;

    KiInsertTimer(Cpu, Timer);
    KeReleaseSpinLockAtDpcLevel(&Cpu->TimerHeapLock);

    KeLowerIrql(Irql);

    return WasQueued;
}

/**
 * Cancels and removes an existing timer from the queue it is placed in.
 */
IONAPI BOOLEAN KeCancelTimer(KTIMER *Timer) {
    assert(0 && "unimpl");
}

/**
 * Just initializes the pairing heap per-CPU
 */
VOID KeInitializeTimerSystem() {
    KCPU *Cpu = KeGetCpu();
    KeInitializeSpinLock(&Cpu->TimerHeapLock);
    Cpu->TimerHeap = NULL;
}
