/**
 * @file include/ion/ke/ps/scheduler.h
 * @brief SCHEDULER object
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_PS_SCHEDULER_H
#define _ION_KE_PS_SCHEDULER_H 1

#include <ion/base.h>
#include <ion/ke/ke/dpc.h>
#include <ion/ke/ke/lock.h>
#include <ion/ion.h>

#define SCHED_NUM_QUEUES        32  // TODO: must be 32 for now since UINT is used for Bitmap
#define SCHED_DEFAULT_QUANTUM   30  // in milliseconds

#define SCHED_TIMESHARED_DISTANCE (PRIORITY_HIGH_USER + 1)


typedef enum _SCHED_QUEUE_TYPE {
    SCHED_QUEUE_NONE,
    SCHED_QUEUE_REALTIME,
    SCHED_QUEUE_TIMESHARED,
    SCHED_QUEUE_IDLE
} SCHED_QUEUE_TYPE;

typedef struct _THREAD_SCHEDULE {
    LIST_ENTRY Entry;
    SCHED_QUEUE_TYPE CurrentQueue;

    ULONGLONG SleepTime;        // relative ms
    ULONGLONG RunTime;          // relative ms
    ULONGLONG LastTime;         // in absolute ms from HalQueryMonotonicTime

    UCHAR RemainingQuantum;

    BOOLEAN Interactive;
    UCHAR CalendarIndex;
    UCHAR SleepBoost;
} THREAD_SCHEDULE;

typedef struct _SCHED_RUNTIME_QUEUE {
    LIST_ENTRY Queues[SCHED_NUM_QUEUES];
    UINT32 Bitmap;
} SCHED_RUNTIME_QUEUE;

typedef struct _SCHED_CALENDAR_QUEUE {
    LIST_ENTRY Queues[SCHED_NUM_QUEUES];
    UINT InsertionIndex;
    UINT RemovalIndex;

    UINT32 Bitmap;
} SCHED_CALENDAR_QUEUE;

typedef struct _SCHEDULER {
    KSPIN_LOCK Lock;

    SCHED_RUNTIME_QUEUE Realtime;
    SCHED_CALENDAR_QUEUE Timeshared;
    SCHED_RUNTIME_QUEUE Idle;
    KDPC Dpc;
} SCHEDULER;

#endif
