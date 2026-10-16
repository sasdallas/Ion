/**
 * @file include/ion/hal.h
 * @brief Ion Hal API
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_HAL_H
#define _ION_HAL_H 1

#include <ion/base.h>

IONHALAPI VOID HalPrintString(const CHAR* String);
IONHALAPI OS_STATUS HalInitSystemConsole();
IONHALAPI VOID HalConsolePrintf(const CHAR* Format, ...);

#endif
