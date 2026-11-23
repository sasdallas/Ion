/**
 * @file include/ion/ke/ke/pushlock.h
 * @brief KPUSHLOCK
 * 
 * Pushlocks in Ke? How scandalous!
 * Pushlocks are like fast mutexes, but they can't be waited on.
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_PUSHLOCK_H
#define _ION_KE_KE_PUSHLOCK_H 1

#include <ion/base.h>
#include <ion/ke/ps/dispatcher.h>
#include <ion/ke/ke/event.h>

typedef UINT_PTR KPUSH_LOCK;

typedef struct _KPUSH_LOCK_WAIT_BLOCK {
    KEVENT SignalEvent;
    LONG ShareCount;
    LONG Flags;
    struct _KPUSH_LOCK_WAIT_BLOCK *Next;
    struct _KPUSH_LOCK_WAIT_BLOCK *Last;
    struct _KPUSH_LOCK_WAIT_BLOCK *Previous;
} __attribute__((aligned(16))) KPUSH_LOCK_WAIT_BLOCK;

#define PUSH_LOCK_FLAGS_SPINNING        0x1
#define PUSH_LOCK_FLAGS_EXCLUSIVE       0x2
#define PUSH_LOCK_FLAGS_UNLOCKED        0x4

#define PUSH_LOCK_LOCKED            BIT(0)
#define PUSH_LOCK_WAITING           BIT(1)
#define PUSH_LOCK_WAKING            BIT(2)
#define PUSH_LOCK_MULTIPLE_SHARED   BIT(3)
#define PUSH_LOCK_MASK              ~(PUSH_LOCK_LOCKED | PUSH_LOCK_WAITING | PUSH_LOCK_WAKING | PUSH_LOCK_MULTIPLE_SHARED)

#endif
