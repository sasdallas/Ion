/**
 * @file include/ion/rtl/dbg.h
 * @brief Debug module code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_RTL_DBG_H
#define _ION_RTL_DBG_H 1

#include <ion/status.h>
#include <ion/types.h>

// RTL/Ke
OS_STATUS DbgInit();
VOID DbgPrintString(const CHAR* String);

// RTL
VOID DbgPrintMsg(const CHAR* Message, ...);

// little helper
#define __HELPER1(x) #x
#define __HELPER2(x) __HELPER1(x)
#define DbgPrint(stmt, ...) DbgPrintMsg("(" __FILE__ ":" __HELPER2(__LINE__)  ") " stmt, ## __VA_ARGS__)

#endif
