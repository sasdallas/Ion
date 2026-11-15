/**
 * @file include/ion/ke/ke/priority.h
 * @brief KPRIORITY
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_PRIORITY_H
#define _ION_KE_KE_PRIORITY_H 1

#include <ion/base.h>

/**
 * Priority definitions stolen from the MINTIA2 operating system
 * OsProcess.hjk: https://github.com/xrarch/mintia2/blob/main/OS/Headers/System/OsProcess.hjk
 * 
 * Works with the ULE scheduler, 1-23 are timeshared and 24-31 are realtime
 */
#define PRIORITY_IDLE           0
#define PRIORITY_LOW_USER       1
#define PRIORITY_DEFAULT        8
#define PRIORITY_RESPONSIVE     10
#define PRIORITY_HIGH_USER      23
#define PRIORITY_LOW_REALTIME   24
#define PRIORITY_MED_REALTIME   28
#define PRIORITY_HIGH_REALTIME  31
#define PRIORITY_MAX            32

typedef UCHAR KPRIORITY;

#endif
