/**
 * @file ionkrnl/ke/semaphore.c
 * @brief Semaphore
 * 
 * @todo THIS IS A STUPID KSEMAPHORE IMPLEMENTATION. DOES NOT SUPPORT BLOCKING.
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
    KeInitializeSpinLock(&Semaphore->Lock);
    Semaphore->Count = Count;
    Semaphore->Limit = Limit;
}

IONAPI LONG KeReleaseSemaphore(KSEMAPHORE *Semaphore, KPRIORITY Boost, LONG Adjustment, BOOLEAN Wait) {
    KIRQL Irql;
    KeAcquireSpinLock(&Semaphore->Lock, &Irql);
    LONG CountLast = InterlockedExchangeAdd(&Semaphore->Count, Adjustment);
    KeReleaseSpinLock(&Semaphore->Lock, Irql);
    return !!CountLast;
}

IONAPI BOOLEAN KeTryAcquireSemaphore(KSEMAPHORE *Semaphore) {
    KIRQL Irql;
    KeAcquireSpinLock(&Semaphore->Lock, &Irql);

    if (Semaphore->Count == 0) {
        KeReleaseSpinLock(&Semaphore->Lock, Irql);
        return FALSE;
    }

    Semaphore->Count -= 1;
    KeReleaseSpinLock(&Semaphore->Lock, Irql);
    return TRUE;
}
