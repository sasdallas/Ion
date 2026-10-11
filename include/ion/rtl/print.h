/**
 * @file include/ion/rtl/print.h
 * @brief RTL print functions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_RTL_PRINT_H
#define _ION_RTL_PRINT_H 1

#include <stdarg.h>

int RtlFormatStringVa(char *buffer, unsigned int length, const char *format, va_list ap);
int RtlFormatString(char *buffer, unsigned int length, const char *format, ...);

#endif
