/**
 * @file ionkrnl/ke/pushlock.c
 * @brief Pushlock
 * 
 * This pushlock interpretation takes inspiration from the ReactOS kernel's ntoskrnl/ex/pushlock.c impl
 * in terms of the Last optimization
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/atomic.h>

/* Configuration */
#define PUSH_LOCK_SPIN_COUNT     256

/* Debug assertions */
#define PUSH_LOCK_DEBUG 1
#if PUSH_LOCK_DEBUG
#define PUSH_LOCK_ASSERT(x) ASSERT(x)
#else
#define PUSH_LOCK_ASSERT(x) (void)(x)
#endif

/* Helpers */
#define PUSH_LOCK_POINTER(x) (KPUSH_LOCK_WAIT_BLOCK*)((((x)) & PUSH_LOCK_MASK))
#define PUSH_LOCK_SHARED(x) (LONG)(((x) & PUSH_LOCK_MASK) >> 4)

/**
 * Find last waiter block in the PushLock's list
 * This has a secondary job which is to finish building the waitblock chain's previous nodes.
 * It assumes that anything that has a set Last pointer has already had the further chain built for it.
 */
static KPUSH_LOCK_WAIT_BLOCK *KiGetLastWaiterBlock(KPUSH_LOCK_WAIT_BLOCK *WaitBlock) {
    KPUSH_LOCK_WAIT_BLOCK *Previous = NULL;
    while (TRUE) {
        WaitBlock->Previous = Previous;
        Previous = WaitBlock;

        // If this is set, we don't need to continue iterating down and building the list
        // further. We have a cached Last pointer, meaning something else already finished
        // constructing the chain.
        if (WaitBlock->Last) {
            return WaitBlock->Last;
        }

        WaitBlock = WaitBlock->Next;
    }
}

/**
 * Wake a pushlock waiter
 * Generally this just sets the UNLOCKED bit on the waiter's flags, then
 * signals the event if the spinning bit is unset.
 */
static VOID KiWakePushLockWaiter(KPUSH_LOCK_WAIT_BLOCK *WaitBlock) {
    LONG Flags = InterlockedOr(&WaitBlock->Flags, PUSH_LOCK_FLAGS_UNLOCKED);
    if ((Flags & PUSH_LOCK_FLAGS_SPINNING) == 0) {
        KeSetEvent(&WaitBlock->SignalEvent, 0);
    }
}

/**
 * Wake a pushlock's pending waiters.
 * Should own WAKING bit before calling this.
 */
static VOID KiWakePushLock(KPUSH_LOCK *PushLock, KPUSH_LOCK Value) {
    PUSH_LOCK_ASSERT(Value & PUSH_LOCK_WAITING);
    PUSH_LOCK_ASSERT(Value & PUSH_LOCK_WAKING);

    while (TRUE) {
        if (Value & PUSH_LOCK_LOCKED) {
            // This pushlock was locked, meaning we shouldn't need to do any work.
            KPUSH_LOCK NewValue = Value & ~(PUSH_LOCK_WAKING);
            NewValue = InterlockedCompareExchangePointer(PushLock, NewValue, Value);
            if (NewValue == Value) {
                return;
            }

            // Lost
            Value = NewValue;
            continue;
        }

        KPUSH_LOCK_WAIT_BLOCK *Head = PUSH_LOCK_POINTER(Value);
        KPUSH_LOCK_WAIT_BLOCK *Last = KiGetLastWaiterBlock(Head);
        KPUSH_LOCK_WAIT_BLOCK *NewLast = Last->Previous;

        if (NewLast && Last->Flags & PUSH_LOCK_FLAGS_EXCLUSIVE) {
            // In this case, the last waiter block is exclusive and there is another one to take its place.
            // Detach Last from the chain.
            NewLast->Next = NULL;
            NewLast->Last = NewLast;
            Head->Last = NewLast->Last;

            KPUSH_LOCK Previous = InterlockedAndPointer(PushLock, ~PUSH_LOCK_WAKING);
            PUSH_LOCK_ASSERT((Previous & PUSH_LOCK_WAKING) && (Previous & PUSH_LOCK_WAITING));

            KiWakePushLockWaiter(Last);
        } else {
            // Just wake them all up. Exclusive locks will just retry.
            // Kill the entire lock first.
            KPUSH_LOCK NewValue = 0;
            NewValue = InterlockedCompareExchangePointer(PushLock, NewValue, Value);
            if (NewValue != Value) {
                Value = NewValue;
                continue;
            }

            while (Last != NULL) {
                KPUSH_LOCK_WAIT_BLOCK *Prev = Last->Previous;
                KiWakePushLockWaiter(Last);
                Last = Prev;
            }
        }

        return;
    }
}

/**
 * Optimize the pushlock waiter chain
 * 
 * Upon insertion, the chain of a pushlock becomes a LIFO list, where you push at the head.
 * This means that callers like KeReleasePushLockXxx have to walk the chain to get the last.
 * We can't do this work directly in wait block insertion because, well, it would be a huge race
 * unless you pinned the waiter blocks. And I'm still larping Windows so whatever.
 */
static VOID KiOptimizePushLock(KPUSH_LOCK *PushLock) {
    KPUSH_LOCK Value = *PushLock;
    while (TRUE) {
        // The lock might've been released by another thread.
        if ((Value & PUSH_LOCK_LOCKED) == 0) {
            // In this case we've effectively been transformed into the perfect state for
            // KiWakePushLock - we own the waking bit and the lock is released.
            return KiWakePushLock(PushLock, Value);
        }

        // Optimize the list by building the last pointer chain.
        KPUSH_LOCK_WAIT_BLOCK *Head = PUSH_LOCK_POINTER(Value);
        KPUSH_LOCK_WAIT_BLOCK *Block = Head;
        KPUSH_LOCK_WAIT_BLOCK *Previous = NULL;
        while (TRUE) {
            Block->Previous = Previous;

            // If the Last pointer is set, this list is already optimized from now on.
            if (Block->Last) {
                // Move this pointer to the head. Since we've already constructed the properly chained list
                // we can leave the head with the skip pointer.
                Head->Last = Block->Last;
                break;
            }

            Previous = Block;
            Block = Block->Next;
        }
    
        // We're done optimizing, release waking.
        KPUSH_LOCK NewValue = Value & ~(PUSH_LOCK_WAKING);
        NewValue = InterlockedCompareExchangePointer(PushLock, NewValue, Value);
        if (NewValue == Value) {
            return;
        }

        Value = NewValue;
    }
}

/**
 * Initialize a pushlock
 * TODO: Should probably just inline this
 */
IONAPI VOID KeInitializePushLock(KPUSH_LOCK *PushLock) {
    *PushLock = 0;
}

/**
 * Acquire a pushlock exclusively (slow path)
 * This is the failure case by KeAcquirePushLock which tries to do a fast bit-set on the locked bit.
 *
 * Callers here need to be running with IRQL <= DISPATCH_LEVEL because it will block
 * on KeWaitForSingleObject.
 */
IONAPI VOID KeAcquirePushLockExclusiveSlow(KPUSH_LOCK *PushLock) {
    KPUSH_LOCK Value = *PushLock; 
    
    // Enter pushlock acquisition loop, we lost the fast case.
    while (TRUE) {
        // If the locked bit is not set here, we can take the lock.
        if ((Value & PUSH_LOCK_LOCKED) == 0) {
            if (InterlockedCompareExchangePointerAcquire(PushLock, Value | PUSH_LOCK_LOCKED, Value) != Value) {
                Value = *PushLock;
                continue;
            }

            break;
        }

        // Create a waitblock
        KPUSH_LOCK_WAIT_BLOCK WaitBlock;
        ASSERT((((UINT_PTR)&WaitBlock) & 0xF) == 0);
        WaitBlock.Flags = PUSH_LOCK_FLAGS_EXCLUSIVE | PUSH_LOCK_FLAGS_SPINNING;
        WaitBlock.Previous = NULL;

        // Check if there is already a waiter
        BOOLEAN DoOptimize = FALSE;
        KPUSH_LOCK NewValue;
        if (Value & PUSH_LOCK_WAITING) {
            // WAITING is set, there is already a water. We need to try and CAS ourselves in.
            WaitBlock.Last = NULL;
            WaitBlock.ShareCount = 0;
            WaitBlock.Next = (KPUSH_LOCK_WAIT_BLOCK*)(Value & PUSH_LOCK_MASK);
         
            // Set WAKING bit here so we can do KiOptimizePushLock
            NewValue = (UINT_PTR)(&WaitBlock) | (Value & PUSH_LOCK_MULTIPLE_SHARED) | PUSH_LOCK_WAITING | PUSH_LOCK_WAKING | PUSH_LOCK_LOCKED;
            if ((Value & PUSH_LOCK_WAKING) == 0) {
                DoOptimize = TRUE;
            }
        } else {
            // We're the first node in the list
            WaitBlock.Last = &WaitBlock;
            WaitBlock.Next = NULL;
            WaitBlock.ShareCount = 0;

            NewValue = (UINT_PTR)(&WaitBlock) | PUSH_LOCK_WAITING | PUSH_LOCK_LOCKED; 
            
            // If the share count is greater than 1, then there is multiple shared and we need to move the value
            // into the waiter block.
            LONG OldShared = (Value & PUSH_LOCK_MASK) >> 4;
            if (OldShared > 1) {
                NewValue |= PUSH_LOCK_MULTIPLE_SHARED;
                WaitBlock.ShareCount = OldShared;
            }
        }

        // Do the insertion
        KPUSH_LOCK Save;
        NewValue = InterlockedCompareExchangePointer(PushLock, NewValue, Value);       
        if (NewValue != Value) {
            Value = *PushLock;
            continue;
        }

        // Prepare us for sleep
        KeInitializeEvent(&WaitBlock.SignalEvent, SynchronizationEvent, FALSE);

        // We have done the insertion, currently we can't be woken up because our
        // WaitBlock has the spinning flag set.
        if (DoOptimize) {
            KiOptimizePushLock(PushLock);
        }

        // Spinning is only useful on SMP builds of Ion, since it gives us a chance to let
        // another CPU core release the lock and try to wake us up
#if _SMP
        for (INT i = 0; i < PUSH_LOCK_SPIN_COUNT; i++) {
            if ((*((volatile LONG*)&WaitBlock.Flags)) & PUSH_LOCK_FLAGS_UNLOCKED) {
                break;
            }

            KeStallExecution();
        }
#endif

        // We're no longer spinning :-(
        LONG Flags = InterlockedAnd(&WaitBlock.Flags, ~(PUSH_LOCK_FLAGS_SPINNING));
        if ((Flags & PUSH_LOCK_FLAGS_UNLOCKED) == 0) {
            OS_STATUS Status = KeWaitForSingleObject(&WaitBlock.SignalEvent, Executive, KernelMode, FALSE, -1);
            ASSERT(Status == STATUS_SUCCESS);
        }

        // Retry
        Value = *PushLock;
    }
}

/**
 * Acquire a shared pushlock. This acquisition can be shared across multiple threads.
 * Do note that it's not guranteed to get the pushlock even if its already acquired shared.
 * To avoid starvation (among other things) shared pushlocks can only *be* shared if there aren't
 * any other exclusive waiters on them.
 */
IONAPI VOID KeAcquirePushLockShared(KPUSH_LOCK *PushLock) {
    KPUSH_LOCK Value = *PushLock;

    while (TRUE) {
        // If it's not locked, then lock it. Genius!
        if (!(Value & PUSH_LOCK_LOCKED)) {
            KPUSH_LOCK NewValue = Value | PUSH_LOCK_LOCKED;

            if (!(Value & PUSH_LOCK_WAITING)) {
                // No one is waiting on this lock, we should increment share count.
                // This is a pretty unlikely race for us to win, since the lock is free for the taking.
                NewValue += (1 << 4);
            }

            NewValue = InterlockedCompareExchangePointerAcquire(PushLock, NewValue, Value);
            if (NewValue == Value) {
                return;
            }

            Value = NewValue;
            continue;
        }

        // If it is locked, maybe it's locked by a shared acquisition without any waiters.
        // We don't want to take the lock if there are already waiters, even if we could steal it.
        if (!(Value & PUSH_LOCK_WAITING) && PUSH_LOCK_SHARED(Value) > 0) {
            KPUSH_LOCK NewValue = (Value + (1 << 4));
        
            NewValue = InterlockedCompareExchangePointerAcquire(PushLock, NewValue, Value);
            if (NewValue == Value) {
                return;
            }

            Value = NewValue;
            continue;
        }

        // Otherwise, we're gonna have to make a waitblock.
        KPUSH_LOCK_WAIT_BLOCK WaitBlock;
        WaitBlock.Flags = PUSH_LOCK_FLAGS_SPINNING;
        WaitBlock.ShareCount = 0;
        WaitBlock.Previous = NULL;

        // Check if there is already a waiter
        BOOLEAN DoOptimize = FALSE;
        KPUSH_LOCK NewValue;
        if (Value & PUSH_LOCK_WAITING) {
            // WAITING is set, there is already a water. We need to try and CAS ourselves in.
            WaitBlock.Last = NULL; // filled in by KiOptimizePushLock or KiGetLastWaiterBlock
            WaitBlock.ShareCount = 0;
            WaitBlock.Next = (KPUSH_LOCK_WAIT_BLOCK*)(Value & PUSH_LOCK_MASK);
         
            // Set WAKING bit here so we can do KiOptimizePushLock
            NewValue = (UINT_PTR)(&WaitBlock) | (Value & PUSH_LOCK_MULTIPLE_SHARED) | PUSH_LOCK_WAITING | PUSH_LOCK_WAKING | PUSH_LOCK_LOCKED;
            if ((Value & PUSH_LOCK_WAKING) == 0) {
                DoOptimize = TRUE;
            }
        } else {
            // We're the first node in the list
            WaitBlock.Last = &WaitBlock;
            WaitBlock.Next = NULL;
            WaitBlock.ShareCount = 0;

            NewValue = (UINT_PTR)(&WaitBlock) | PUSH_LOCK_WAITING | PUSH_LOCK_LOCKED; 
        }

        // Try to apply us to the list
        if (InterlockedCompareExchangePointer(PushLock, NewValue, Value) != Value) {
            Value = *PushLock;
            continue;
        }

        // Prepare us for sleep
        KeInitializeEvent(&WaitBlock.SignalEvent, SynchronizationEvent, FALSE);

        // We have done the insertion, currently we can't be woken up because our
        // WaitBlock has the spinning flag set.
        if (DoOptimize) {
            KiOptimizePushLock(PushLock);
        }

        // Spinning is only useful on SMP builds of Ion, since it gives us a chance to let
        // another CPU core release the lock and try to wake us up
#if _SMP
        for (INT i = 0; i < PUSH_LOCK_SPIN_COUNT; i++) {
            if ((*((volatile LONG*)&WaitBlock.Flags)) & PUSH_LOCK_FLAGS_UNLOCKED) {
                break;
            }

            KeStallExecution();
        }
#endif

        // We're no longer spinning :-(
        LONG Flags = InterlockedAnd(&WaitBlock.Flags, ~(PUSH_LOCK_FLAGS_SPINNING));
        if ((Flags & PUSH_LOCK_FLAGS_UNLOCKED) == 0) {
            OS_STATUS Status = KeWaitForSingleObject(&WaitBlock.SignalEvent, Executive, KernelMode, FALSE, -1);
            ASSERT(Status == STATUS_SUCCESS);
        }

        // Retry
        Value = *PushLock;
    }
}

/**
 * Release a shared pushlock. Drops the lock entirely at the last reference.
 */
IONAPI VOID KeReleasePushLockShared(KPUSH_LOCK *PushLock) {
    KPUSH_LOCK Value = *PushLock;
    BOOLEAN ReleasedShare = FALSE;

    while (TRUE) {
        PUSH_LOCK_ASSERT(Value & PUSH_LOCK_LOCKED);

        // If nothing is waiting, the upper bits contain the shared count.
        // Decrement one reference and drop the lock entirely if it is the last one.
        if ((Value & PUSH_LOCK_WAITING) == 0) {
            KPUSH_LOCK NewValue = PUSH_LOCK_SHARED(Value) > 1 ? Value - (1 << 4) : 0;

            NewValue = InterlockedCompareExchangePointer(PushLock, NewValue, Value);
            if (NewValue != Value) {
                Value = NewValue;
                continue;
            }

            return;
        }

        // Otherwise there are waiters, which means the share count has been deferred to the oldest wait block.
        if ((Value & PUSH_LOCK_MULTIPLE_SHARED) && !ReleasedShare) {
            // We don't own the link and thus can't modify anything used by the optimizer.
            KPUSH_LOCK_WAIT_BLOCK *Oldest = PUSH_LOCK_POINTER(Value);
            while (Oldest->Next != NULL) {
                if (Oldest->Last) {
                    Oldest = Oldest->Last;
                    break;
                }

                Oldest = Oldest->Next;
            }

            LONG Remaining = InterlockedDecrement(&Oldest->ShareCount);
            PUSH_LOCK_ASSERT(Remaining >= 0);
            if (Remaining > 0) {
                return;
            }

            ReleasedShare = TRUE;
            Value = *PushLock;
        }

        BOOLEAN DoWake = (Value & PUSH_LOCK_WAITING) && !(Value & PUSH_LOCK_WAKING);

        KPUSH_LOCK NewValue = Value & ~(PUSH_LOCK_LOCKED | PUSH_LOCK_MULTIPLE_SHARED);
        if (DoWake) NewValue |= PUSH_LOCK_WAKING;

        // Do the CAS
        if (InterlockedCompareExchangePointer(PushLock, NewValue, Value) == Value) {
            // Release was successful and we own waking, therefore if we need to we can wakeup the pushlock.
            if (DoWake) KiWakePushLock(PushLock, NewValue);
            break;
        }

        // Try, try again..
        Value = *PushLock;   
    }
}

/**
 * Release an exclusive pushlock.
 */
IONAPI VOID KeReleasePushLockExclusive(KPUSH_LOCK *PushLock) {
    KPUSH_LOCK Value = *PushLock;

    while (TRUE) {
        // Standard practice
        PUSH_LOCK_ASSERT(Value & PUSH_LOCK_LOCKED);

        // Exclusively acquired locks have no shared count, any shared acquisitions immediately go to waiting.
        PUSH_LOCK_ASSERT(((Value & PUSH_LOCK_MASK) == 0) || (Value & PUSH_LOCK_WAITING));

        BOOLEAN DoWake = (Value & PUSH_LOCK_WAITING) && !(Value & PUSH_LOCK_WAKING);

        KPUSH_LOCK NewValue = (Value & ~(PUSH_LOCK_LOCKED));
        if (DoWake) NewValue |= PUSH_LOCK_WAKING;

        // Do the CAS
        if (InterlockedCompareExchangePointer(PushLock, NewValue, Value) == Value) {
            // Release was successful and we own waking, therefore if we need to we can wakeup the pushlock.
            if (DoWake) KiWakePushLock(PushLock, NewValue);
            break;
        }

        // Try, try again..
        Value = *PushLock;
    }
}
