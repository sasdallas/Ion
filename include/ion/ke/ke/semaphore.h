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
#include <ion/ke/ps/dispatcher.h>

typedef struct _KSEMAPHORE {
    DISPATCH_HEADER Header;
    LONG Limit;
} KSEMAPHORE;

#endif
