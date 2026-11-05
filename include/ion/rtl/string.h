/**
 * @file include/ion/rtl/string.h
 * @brief String functions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_RTL_STRING_H
#define _ION_RTL_STRING_H 1

#include <ion/base.h>


INT RtlCompareString(CHAR* String1, CHAR* String2, SIZE_T Size);
BOOLEAN RtlStringEquals(CHAR* String1, CHAR* String2);
SIZE_T RtlStringLength(CHAR *String);

VOID RtlCopyStringN(CHAR* Destination, const CHAR* Source, SIZE_T MaxSize);

#endif
