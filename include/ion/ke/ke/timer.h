/**
 * @file include/ion/ke/ke/timer.h
 * @brief KTIMER
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_TIMER_H
#define _ION_KE_KE_TIMER_H 1

#include <ion/base.h>
#include <ion/ke/ke/dpc.h>
#include <ion/ke/ps/dispatcher.h>

typedef struct _KTIMER {
    DISPATCH_HEADER Header;
    UINT64 DueTime;
    LONG Period;
    KDPC *Dpc;
    
    BOOLEAN IsQueued;
    LIST_ENTRY Children;
    LIST_ENTRY Entry;
} KTIMER;

typedef enum _KTIMER_TYPE {
    NotificationTimer,
    SynchronizationTimer
} KTIMER_TYPE;

#endif
