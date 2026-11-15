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

typedef struct _DISPATCH_HEADER {
    KSPIN_LOCK Spinlock;
    ULONG SignalCount;
    ULONG WaiterCount;
    LIST_ENTRY WaitListHead;
} DISPATCH_HEADER;

#define PsLockDispatcher(Dispatcher) KeAcquireSpinLock(&(Dispatcher)->Spinlock)
#define PsUnlockDispatcher(Dispatcher, Irql) KeReleaseSpinLock(&(Dispatcher)->Spinlock, (Irql))

static inline VOID PsInitializeDispatcher(DISPATCH_HEADER *Header) {
    KeInitializeSpinLock(&Header->Spinlock);
    Header->SignalCount = 0;
    Header->WaiterCount = 0;
    InitializeListHead(&Header->WaitListHead);
}

#endif
