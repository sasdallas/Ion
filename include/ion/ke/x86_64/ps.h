/**
 * @file include/ion/ke/x86_64/ps.h
 * @brief Processor context
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

typedef struct _THREAD_CONTEXT {
    UINT64 R15;
    UINT64 R14;
    UINT64 R13;
    UINT64 R12;
    UINT64 Rdi;
    UINT64 Rsi;
    UINT64 Rbp;
    UINT64 Rbx;
    UINT64 ReturnAddress;
} THREAD_CONTEXT;

#endif
