/**
 * @file include/ion/ke/ke/semaphore.h
 * @brief KSEMAPHORE
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_SEMAPHORE_H
#define _ION_KE_KE_SEMAPHORE_H 1

#include <ion/base.h>

typedef struct _KSEMAPHORE {
    KSPIN_LOCK Lock;
    LONG Count;
    LONG Limit;
} KSEMAPHORE;

#endif
