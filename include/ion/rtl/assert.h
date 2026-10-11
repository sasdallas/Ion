/**
 * @file include/ion/rtl/assert.h
 * @brief assert
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_RTL_ASSERT_H
#define _ION_RTL_ASSERT_H 1

#include <ion/base.h>

NO_RETURN VOID RtlAssertFail(CHAR *Statement, CHAR *File, INT Line, const CHAR *Function);

#define ASSERT(statement) ((void)((statement) || (RtlAssertFail(#statement, __FILE__, __LINE__, __func__), 0)))
#define assert ASSERT

#endif
