/**
 * @file include/ion/ke/ke_types.h
 * @brief Contains all Ke types
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_TYPES_H
#define _ION_KE_KE_TYPES_H 1

#include <ion/base.h>

#include <ion/ke/ke/processor_mode.h>
#include <ion/ke/ke/irql.h>
#include <ion/ke/ke/lock.h>
#include <ion/ke/ke/semaphore.h>
#include <ion/ke/ke/priority.h>
#include <ion/ke/ke/dpc.h>
#include <ion/ke/ke/wait.h>
#include <ion/ke/ke/timer.h>
#include <ion/ke/ke/event.h>

#include <ion/ke/ps/scheduler.h>
#include <ion/ke/ps/thread.h>
#include <ion/ke/ps/process.h>

#define KeIrqlPending(x) (1 << ((x) - 1))

typedef struct _KCPU {
    struct _KCPU *Self;
    
    // The logical ID is assigned by the kernel and the hardware ID is impl-defined
    UINT32 HardwareId;
    UINT32 LogicalId;

    // Current IRQL of the CPU
    KIRQL Irql;
    
    // Software interrupt pending
    LONG PendingSoftInterrupt;

    // The current thread active on the system
    THREAD *CurrentThread;

    // Locked under Scheduler.Lock and shouldn't be modified
    THREAD *NextThread;

    // Idle processor + thread
    THREAD *IdleThread;
    PROCESS *IdleProcess;

    // Wait clean list
    THREAD *PendingUnwaitCleanupList;

    // CPU scheduling information
    SCHEDULER Scheduler;

    // DPC queue (interrupts must be disabled)
    LIST_ENTRY DpcListHead;

    // Pairing heap of timers
    KTIMER *TimerHeap;
    KSPIN_LOCK TimerHeapLock;
} KCPU;

#endif
