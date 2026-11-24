/**
 * @file include/ion/ke/ps.h
 * @brief Ps (process system) 
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_PS_H
#define _ION_KE_PS_H 1

#include <ion/base.h>

#include <ion/ke/ps/process.h>
#include <ion/ke/ps/thread.h>
#include <ion/ke/ps/scheduler.h>
#include <ion/ke/ps/dispatcher.h>
#include <ion/ke/ob.h>

#ifdef __x86_64__
#include <ion/ke/x86_64/ps.h>
#else
#error impl architecture
#endif

extern PROCESS *PsSystemProcess;

extern OBJECT_TYPE *PsProcessObjectType;
extern OBJECT_TYPE *PsThreadObjectType;

#define PsLockThread(Thread) ({ KIRQL Irql; KeAcquireSpinLock(&(Thread)->Spinlock, &Irql); Irql; })
#define PsUnlockThread(Thread, Irql) ({ KeReleaseSpinLock(&(Thread)->Spinlock, (Irql)); })
#define PsLockThreadElevated(Thread) KeAcquireSpinLockAtDpcLevel(&(Thread)->Spinlock)
#define PsUnlockThreadElevated(Thread) KeReleaseSpinLockAtDpcLevel(&(Thread)->Spinlock)
#define PsTryLockThreadElevated(Thread) KeTryAcquireSpinLockAtDpcLevel(&(Thread)->Spinlock)

IONAPI PROCESS *PsGetCurrentProcess();
IONAPI OS_STATUS PsCreateThread(PROCESS *Process, VOID (*StartRoutine)(VOID *Context), VOID *Context, THREAD **ThreadOutput);
IONAPI VOID PsYieldThread();
IONAPI VOID PsYieldThreadLocked(); // enter with thread lock held, exits with thread lock released (IRQL is unchanged!)
IONAPI VOID PsReadyThread(THREAD *Thread);
IONAPI VOID PsTickScheduler();
IONAPI NO_RETURN VOID PsExitThread();
NO_RETURN VOID PsSwitchToNextThread(); // Don't use

VOID PsInitializeProcess(PROCESS *Process, PROCESS *Parent, CHAR *ImageName, KPRIORITY Priority);

OS_STATUS PsInitialize();
OS_STATUS PsInitializeProcessManager();
OS_STATUS PsInitializeSystemProcess();
OS_STATUS PsInitializeScheduler();
OS_STATUS PsInitializeIdleProcess();
OS_STATUS PsInitializeIdleThread();

#endif
