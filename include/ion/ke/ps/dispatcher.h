/**
 * @file include/ion/ke/ps/dispatcher.h
 * @brief Dispatcher
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_PS_DISPATCHER_H
#define _ION_KE_PS_DISPATCHER_H

#include <ion/base.h>
#include <ion/list.h>
#include <ion/ke/ke/lock.h>

// In my never ceasing attempts to not fully larp instead of introducing Header->Type fields we introduce
// Header->Satisfaction fields for KiSatisfyWait()..
typedef enum _DISPATCH_SATISFACTION {
    SatisfyNone = 0,            // Don't do anything to SignalCount
    SatisfyTakeOne,             // Subtract one from SignalCount
    SatisfyReset,               // Reset SignalCount to zero
} DISPATCH_SATISFACTION;

typedef struct _DISPATCH_HEADER {
    KSPIN_LOCK Spinlock;
    ULONG SignalCount;
    LIST_ENTRY WaitListHead;
    DISPATCH_SATISFACTION Satisfaction;
} DISPATCH_HEADER;

#define PsLockDispatcher(Dispatcher, Irql) KeAcquireSpinLock(&(Dispatcher)->Spinlock, (Irql))
#define PsUnlockDispatcher(Dispatcher, Irql) KeReleaseSpinLock(&(Dispatcher)->Spinlock, (Irql))
#define PsLockDispatcherElevated(Dispatcher) KeAcquireSpinLockAtDpcLevel(&(Dispatcher)->Spinlock)
#define PsUnlockDispatcherElevated(Dispatcher) KeReleaseSpinLockAtDpcLevel(&(Dispatcher)->Spinlock)

extern VOID KeInitializeSpinLock(KSPIN_LOCK *SpinLock);

static inline VOID PsInitializeDispatcher(DISPATCH_HEADER *Header, DISPATCH_SATISFACTION Satisfaction) {
    KeInitializeSpinLock(&Header->Spinlock);
    Header->SignalCount = 0;
    Header->Satisfaction = Satisfaction;
    InitializeListHead(&Header->WaitListHead);
}

#endif
