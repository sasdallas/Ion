/**
 * @file include/ion/ke/ke.h
 * @brief Ion core kernel subsystems (Ke)
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_H
#define _ION_KE_KE_H 1

#include <ion/base.h>

#include <ion/ke/ke_types.h>
#include <ion/ke/lpb.h>

#ifdef __x86_64__
#include <ion/ke/x86_64/ke.h>
#else
#error unknown arch
#endif

extern LOADER_PARAMETER_BLOCK *KeLoaderParameterBlock;

/* Ke functions begin */
IONAPI VOID KeInitializeSpinLock(KSPIN_LOCK *SpinLock);
IONAPI VOID KeAcquireSpinLock(KSPIN_LOCK *SpinLock, KIRQL *OldIrql);
IONAPI VOID KeReleaseSpinLock(KSPIN_LOCK *SpinLock, KIRQL NewIrql);

IONAPI NO_RETURN VOID KeBugCheckWithMessage(const CHAR* Message, ...);

#endif
