/**
 * @file ionkrnl/ke/lock.c
 * @brief KeAcquireSpinLock and KeReleaseSpinLock family
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>

IONAPI VOID KeInitializeSpinLock(KSPIN_LOCK *SpinLock) {
    __atomic_clear(SpinLock, __ATOMIC_RELEASE);
}

IONAPI VOID KeAcquireSpinLock(KSPIN_LOCK *SpinLock, KIRQL *OldIrql) {
    KeRaiseIrql(DISPATCH_LEVEL, OldIrql);
    while (__atomic_test_and_set(SpinLock, __ATOMIC_ACQUIRE)) {
        KeStallExecution();
    }
}

IONAPI VOID KeReleaseSpinLock(KSPIN_LOCK *SpinLock, KIRQL NewIrql) {
    __atomic_clear(SpinLock, __ATOMIC_RELEASE);
    KeLowerIrql(NewIrql);
}

IONAPI VOID KeAcquireSpinLockAtDpcLevel(KSPIN_LOCK *SpinLock) {
    while (__atomic_test_and_set(SpinLock, __ATOMIC_ACQUIRE)) {
        KeStallExecution();
    }
}

IONAPI VOID KeReleaseSpinLockAtDpcLevel(KSPIN_LOCK *SpinLock) {
    __atomic_clear(SpinLock, __ATOMIC_RELEASE);
}
