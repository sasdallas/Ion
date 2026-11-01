/**
 * @file include/ion/ke/ps/thread.h
 * @brief THREAD object
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_PS_THREAD_H
#define _ION_KE_PS_THREAD_H 1

#include <ion/base.h>
#include <ion/list.h>

#include <ion/ke/ps/scheduler.h>

// TODO Fix these weird header dependencies
#ifdef __x86_64__
#include <ion/ke/x86_64/ps.h>
#else
#error impl architecture
#endif

struct _PROCESS;
struct _THREAD_CONTEXT;

typedef enum _THREAD_STATE {
    PsThreadInitialized,            // Thread is created but not yet ready to run
    PsThreadReady,                  // Thread is sitting in a scheduler queue
    PsThreadStandby,                // We up
    PsThreadRunning,                // Thread is executing now
    PsThreadTerminated,             // DEAD!
    PsThreadWaiting,                // Waiting on some resource
} THREAD_STATE;

typedef ULONG THREAD_ID;

typedef struct _THREAD {
    KSPIN_LOCK Spinlock;
    THREAD_ID ThreadId;
    THREAD_STATE ThreadState;

    struct _PROCESS *Process;
    LIST_ENTRY ProcEntry;

    THREAD_SCHEDULE Schedule;

    struct _THREAD_CONTEXT Context;
    VOID *KernelStack;
} THREAD;

#endif
