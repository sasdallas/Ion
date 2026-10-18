/**
 * @file include/ion/ke/ps/process.h
 * @brief PROCESS object
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_PS_PROCESS_H
#define _ION_KE_PS_PROCESS_H 1

#include <ion/base.h>
#include <ion/list.h>

typedef ULONG PROCESS_ID;

typedef struct _PROCESS {
    PROCESS_ID Pid;
    LIST_ENTRY ThreadList;
} PROCESS;

#endif
