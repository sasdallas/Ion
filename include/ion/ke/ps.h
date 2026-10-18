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

#ifdef __x86_64__
#include <ion/ke/x86_64/ps.h>
#else
#error impl architecture
#endif

#include <ion/ke/ps/process.h>
#include <ion/ke/ps/thread.h>
#include <ion/ke/ob.h>

extern PROCESS *PsSystemProcess;

extern OBJECT_TYPE *PsProcessObjectType;
extern OBJECT_TYPE *PsThreadObjectType;

IONAPI PROCESS *PsGetCurrentProcess();
IONAPI VOID PsYieldThread();
IONAPI NO_RETURN VOID PsExitThread();

OS_STATUS PsInitialize();
OS_STATUS PsInitializeSystemProcess();

#endif
