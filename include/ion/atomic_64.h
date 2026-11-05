/**
 * @file include/ion/atomic_64.h
 * @brief Interlocked64 operations
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_ATOMIC_64_H
#define _ION_ATOMIC_64_H 1

#include <ion/base.h>

// TODO: portability, these all use the GCC __atomic builtins

// returns value after operation
static inline LONG64 InterlockedIncrement64(volatile LONG64 *Addend) {
    return __atomic_add_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

// returns value after operation
static inline LONG64 InterlockedDecrement64(volatile LONG64 *Addend) {
    return __atomic_sub_fetch(Addend, 1, __ATOMIC_SEQ_CST);
}

// returns value before operation
static inline LONG64 InterlockedExchange64(volatile LONG64 *Target, LONG64 Value) {
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}

// returns value before operation
static inline LONG64 InterlockedExchangeAdd64(volatile LONG64 *Addend, LONG64 Value) {
    return __atomic_fetch_add(Addend, Value, __ATOMIC_SEQ_CST);
}

// returns previous value
static inline LONG64 InterlockedCompareExchange64(volatile LONG64 *Destination, LONG64 Exchange, LONG64 Comparand) {
    LONG64 Expected = Comparand;
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
static inline LONG64 InterlockedAnd64(volatile LONG64 *Target, LONG64 Value) {
    return __atomic_fetch_and(Target, Value, __ATOMIC_SEQ_CST);
}

// returns value before operation
static inline LONG64 InterlockedOr64(volatile LONG64 *Target, LONG64 Value) {
    return __atomic_fetch_or(Target, Value, __ATOMIC_SEQ_CST);
}


#endif
