/**
 * @file include/ion/ke/x86_64/ps.h
 * @brief Process system
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_X86_64_PS_H
#define _ION_KE_X86_64_PS_H 1

#include <ion/base.h>

#ifndef _ION_KE_PS_THREAD_H
#error Include ion/ke/ps/thread.h first!
#endif

extern VOID PspThreadEntry();

FORCE_INLINE VOID PspInitializeThreadContext(THREAD *Thread, VOID *StartRoutine, VOID *Context) {
    RtlZeroMemory(&Thread->Context, sizeof(Thread->Context));
    Thread->Context.R12 = (UINT64)StartRoutine;
    Thread->Context.R13 = (UINT64)Context;
    Thread->Context.Rsp = (((UINT64)Thread->KernelStack + 24576) & ~(UINT64)15) - 8;
    Thread->Context.Rip = (UINT64)PspThreadEntry;

    UINT_PTR Fpu = ((UINT_PTR)Thread->Context.FpuState + 15) & ~(UINT_PTR)15;
    asm volatile ("fxsave64 (%0)" :: "r"(Fpu) : "memory");
}

#endif
