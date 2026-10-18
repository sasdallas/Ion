/**
 * @file include/ion/atomic.h
 * @brief Interlocked operations
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_ATOMIC_H
#define _ION_ATOMIC_H 1

#include <ion/base.h>
#include <ion/atomic_64.h>

// TODO: portability, these all use the GCC __atomic builtins

// returns value after operation
static inline LONG InterlockedIncrement(volatile LONG *Addend) {
    return __atomic_add_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

// returns value after operation
static inline LONG InterlockedDecrement(volatile LONG *Addend) {
    return __atomic_sub_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

// returns value before operation
static inline LONG InterlockedExchange(volatile LONG *Target, LONG Value) {
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}

// returns value before operation
static inline LONG InterlockedExchangeAdd(volatile LONG *Addend, LONG Value) {
    return __atomic_fetch_add(Addend, Value, __ATOMIC_SEQ_CST);
}

// returns previous value
static inline LONG InterlockedComapreExchange(volatile LONG *Destination, LONG Exchange, LONG Comparand) {
    LONG Expected = Comparand;
    __atomic_compare_exchange_n(
        Destination,
        &Expected,
        Exchange,
        0,
        __ATOMIC_SEQ_CST,
        __ATOMIC_SEQ_CST
    );

    return Expected;
}

// returns value before operation
static inline LONG InterlockedAnd(volatile LONG *Target, LONG Value) {
    return __atomic_fetch_and(Target, Value, __ATOMIC_SEQ_CST);
}

// returns value before operation
static inline LONG InterlockedOr(volatile LONG *Target, LONG Value) {
    return __atomic_fetch_or(Target, Value, __ATOMIC_SEQ_CST);
}

#endif
