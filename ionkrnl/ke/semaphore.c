/**
 * @file ionkrnl/ke/semaphore.c
 * @brief Semaphore
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ion.h>

IONAPI VOID KeInitializeSemaphore(KSEMAPHORE *Semaphore, LONG Count, LONG Limit) {
    PsInitializeDispatcher(&Semaphore->Header);
    Semaphore->Header.SignalCount = (ULONG)Count;
    Semaphore->Limit = Limit;
}

IONAPI LONG KeReleaseSemaphore(KSEMAPHORE *Semaphore, KPRIORITY Boost, LONG Adjustment, BOOLEAN Wait) {
    if (Wait) {
        KeBugCheckWithMessage("KeReleaseSemaphore(): Wait is not supported");
    }

    DISPATCH_HEADER *Header = &Semaphore->Header;
    KIRQL Irql;
    KeAcquireSpinLock(&Header->Spinlock, &Irql);

    LONG PreviousCount = (LONG)Header->SignalCount;
    if (Adjustment > Semaphore->Limit - PreviousCount) {
        KeBugCheckWithMessage("KeReleaseSemaphore(): semaphore limit exceeded");
    }

    for (LONG i = 0; i < Adjustment; i++) {
        if (KeProcessObjectWaiters(Semaphore, FALSE, STATUS_SUCCESS) == 0) {
            Header->SignalCount += 1;
        }
    }

    KeReleaseSpinLock(&Header->Spinlock, Irql);
    return PreviousCount;
}

IONAPI BOOLEAN KeTryAcquireSemaphore(KSEMAPHORE *Semaphore) {
    DISPATCH_HEADER *Header = &Semaphore->Header;
    KIRQL Irql;
    KeAcquireSpinLock(&Header->Spinlock, &Irql);

    if (Header->SignalCount == 0) {
        KeReleaseSpinLock(&Header->Spinlock, Irql);
        return FALSE;
    }

    Header->SignalCount -= 1;
    KeReleaseSpinLock(&Header->Spinlock, Irql);
    return TRUE;
}
