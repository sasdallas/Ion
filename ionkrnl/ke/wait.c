/**
 * @file ionkrnl/ke/wait.c
 * @brief KeWaitForXxx manager
 * 
 * The design of this waiter is modified from MINTIA2's design and Windows' flow.
 * 
 * Effectively the core of it is that it supports fine-grained locking rather than a big lock
 * around the entire kernel dispatcher. It uses CAS to signal a wait completion on the thread too.
 * That's about it though, otherwise this is pretty bog standard.
 * 
 * @copyright
 * This design is inspired by the MINTIA2 operating system.
 * https://github.com/xrarch/mintia2/blob/main/OS/Executive/Ke/KeWait.jkl
 * 
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ke/ps.h>

static VOID KiDequeueWaitBlock(KWAIT_BLOCK *WaitBlock) {
    DISPATCH_HEADER *Header = WaitBlock->Header;

    if (WaitBlock->Thread) {
        WaitBlock->Thread = NULL;
        Header->WaiterCount -= 1;
        RemoveEntryList(&WaitBlock->Entry);
    }
}

// expects locked header
static VOID KiInsertWaitBlock(DISPATCH_HEADER *Header, KWAIT_BLOCK *WaitBlock) {
    WaitBlock->Header = Header;
    InsertTailList(&Header->WaitListHead, &WaitBlock->Entry);
    Header->WaiterCount += 1;
}

static BOOLEAN KiIsDispatcherSignalled(DISPATCH_HEADER *Header) {
    return Header->SignalCount > 0;
}

static BOOLEAN KiTryCompleteWait(THREAD *Thread, OS_STATUS WaitStatus) {
    return InterlockedCompareExchange(
        &Thread->WaitStatus, WaitStatus, STATUS_WAIT_PLACEHOLDER
    ) == STATUS_WAIT_PLACEHOLDER;
}

static VOID KiCleanupThread(THREAD *Thread, DISPATCH_HEADER *LockedHeader) {
    for (ULONG i = 0; i < Thread->WaitCount; i++) {
        KWAIT_BLOCK *WaitBlock = &Thread->WaitBlockArray[i];
        DISPATCH_HEADER *Header = WaitBlock->Header;

        if (Header != LockedHeader) PsLockDispatcherElevated(Header);
        KiDequeueWaitBlock(WaitBlock);
        if (Header != LockedHeader) PsUnlockDispatcherElevated(Header);
    }

    Thread->WaitCount = 0;
}

static VOID KiTryCleanupThread(THREAD *Thread, DISPATCH_HEADER *LockedHeader) {
    // XXX: The caller holds LockedHeader, so cleaning a multiple-object wait here
    //      would acquire another dispatcher lock while holding it.
    if (Thread->WaitCount == 1 && PsTryLockThreadElevated(Thread)) {
        KiCleanupThread(Thread, LockedHeader);
        PsUnlockThreadElevated(Thread);
        PsReadyThread(Thread);
        return;
    }

    // TODO This should be made fifo
    KCPU *Cpu = KeGetCpu();
    Thread->PendingUnwaitNext = Cpu->PendingUnwaitCleanupList;
    Cpu->PendingUnwaitCleanupList = Thread;

    KeRequestSoftwareInterrupt(DISPATCH_LEVEL);
}

VOID KiCleanupPendingWaiters() {
    KCPU *Cpu = KeGetCpu();
    if (Cpu->PendingUnwaitCleanupList == NULL) {
        return;
    }

    DbgPrint("KiCleanupPendingWaiters()\n");

    THREAD *Thread = Cpu->PendingUnwaitCleanupList;
    Cpu->PendingUnwaitCleanupList = NULL;
    
    while (Thread) {
        THREAD *Next = Thread->PendingUnwaitNext;
        PsLockThreadElevated(Thread);
        KiCleanupThread(Thread, NULL);
        PsUnlockThreadElevated(Thread);

        PsReadyThread(Thread);

        Thread = Next;
    }
}

/**
 * Wakeup people on the object, expects object locked.
 */
INT KeProcessObjectWaiters(VOID *Object, BOOLEAN WakeAll, OS_STATUS Status) {
    DISPATCH_HEADER *Header = Object;

    INT Processed = 0;
    while (!IsListEmpty(&Header->WaitListHead)) {
        LIST_ENTRY *Entry = Header->WaitListHead.Flink;
        KWAIT_BLOCK *WaitBlock = CONTAINING_RECORD(Entry, KWAIT_BLOCK, Entry);
        OS_STATUS WakeupStatus = Status;

        // Show that the waitblock is yoinked
        THREAD *Thread = WaitBlock->Thread;
        WaitBlock->Thread = NULL;

        // Remove it from the list
        RemoveEntryList(&WaitBlock->Entry);
        Header->WaiterCount -= 1;

        // STATUS_SUCCESS is generic, and it means that we should give them the index of the signaller
        if (WakeupStatus == STATUS_SUCCESS) {
            WakeupStatus = WaitBlock->Index;
        }

        if (KiTryCompleteWait(Thread, WakeupStatus) == TRUE) {
            // We woke up the thread!
            Processed += 1;
            KiTryCleanupThread(Thread, Header);

            if (!WakeAll) {
                break;
            }
        }

    }

    return Processed;
}

/**
 * Wait for multiple objects to become signalled or for timeout to expire
 */
OS_STATUS KeWaitForMultipleObjects(
    ULONG Count,
    VOID **Objects,
    KWAIT_REASON WaitReason,
    KPROCESSOR_MODE WaitMode,
    BOOLEAN Alertable,
    INT Timeout,
    KWAIT_BLOCK *WaitBlockArray
)
{
    ASSERT(Timeout == -1 && "Timeout != -1 isn't supported at the moment :-)");
    ASSERT(Count < MAX_WAIT_BLOCKS); 

    THREAD *Thread = KeGetCurrentThread();

    // Go up to DISPATCH_LEVEL to prevent others from messing with us
    KIRQL WaitIrql;
    KeRaiseIrql(DISPATCH_LEVEL, &WaitIrql);

    PsLockThreadElevated(Thread);

    // Always use the user-provided WaitBlockArray if they want to
    KWAIT_BLOCK *WaitBlocks;
    if (WaitBlockArray) {
        WaitBlocks = WaitBlockArray;
    } else {
        if (Count > THREAD_WAIT_BLOCKS) {
            KeBugCheckWithMessage("KeWaitForMultipleObjects(): Too many waitblocks but an array wasn't given!");
        }

        WaitBlocks = Thread->WaitBlocks;
    }
        
    // WAIT_PLACEHOLDER is an idea "borrowed" from MINTIA's wait design
    Thread->WaitStatus = STATUS_WAIT_PLACEHOLDER;
    Thread->WaitCount = Count;
    Thread->WaitBlockArray = WaitBlocks;

    // Iterate through each dispatch header
    for (ULONG i = 0; i < Count; i++) {
        KWAIT_BLOCK *WaitBlock = &WaitBlocks[i];
        
        DISPATCH_HEADER *Header = (DISPATCH_HEADER*)Objects[i];
        PsLockDispatcherElevated(Header);
        
        if (KiIsDispatcherSignalled(Header)) {
            // Only the winner may consume this signal
            BOOLEAN Won = KiTryCompleteWait(Thread, i);
            if (Won) {
                Header->SignalCount -= 1;
            }

            PsUnlockDispatcherElevated(Header);

            // Only the first i blocks were inserted into dispatcher lists.
            Thread->WaitCount = i;
            if (Won) {
                KiCleanupThread(Thread, NULL);
                PsUnlockThreadElevated(Thread);
                KeLowerIrql(WaitIrql);
                return i;
            }

            // The winning signaler will ready us after we release the thread
            // lock in PsYieldThreadLocked
            Thread->ThreadState = PsThreadWaiting;
            PsYieldThreadLocked();
            KeLowerIrql(WaitIrql);
            return Thread->WaitStatus;
        }

        // Otherwise we should add ourselves
        WaitBlock->Thread = Thread;
        WaitBlock->Index = i;

        KiInsertWaitBlock(Header, WaitBlock);
        PsUnlockDispatcherElevated(Header);
    }

    // Nothing signalled us, so continue with the wait
    Thread->ThreadState = PsThreadWaiting;
    PsYieldThreadLocked();

    // Something signalled us, and they have already cleaned us up probably. We're good to return
    KeLowerIrql(WaitIrql);
    return Thread->WaitStatus;
}

/**
 * Waits for a single object's dispatch header to become signalled
 */
OS_STATUS KeWaitForSingleObject(
    VOID *Object,
    KWAIT_REASON WaitReason,
    KPROCESSOR_MODE WaitMode,
    BOOLEAN Alertable,
    INT Timeout
)
{
    VOID *Objects[] = { Object };
    
    return KeWaitForMultipleObjects(
        1,
        Objects,
        WaitReason,
        WaitMode,
        Alertable,
        Timeout,
        NULL
    );
}
