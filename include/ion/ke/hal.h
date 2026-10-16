/**
 * @file include/ion/ke/hal.h
 * @brief Internal Ke/HAL manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_HAL_H
#define _ION_KE_HAL_H 1

#include <ion/base.h>

typedef struct _HAL_VTABLE {
    OS_STATUS (*InitSystemConsole)();
    VOID (*PrintString)(const CHAR* String);
} HAL_VTABLE;

IONAPI VOID HalSetVtable(HAL_VTABLE *Vtable);

#endif
