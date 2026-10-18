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
#include <ion/ke/ps/thread.h>
#include <ion/ke/ke/processor_mode.h>

typedef UINT_PTR KSPIN_LOCK;

typedef struct _KCPU {
    struct _KCPU *Self;
    UINT32 HardwareId;
    UINT32 LogicalId; // assigned by the kernel
    THREAD *CurrentThread;
} KCPU;

#endif
