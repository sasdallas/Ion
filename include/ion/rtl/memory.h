/**
 * @file include/ion/rtl/memory.h
 * @brief Memory management functions (RtlZeroMemory and friends)
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_RTL_MEMORY_H
#define _ION_RTL_MEMORY_H 1 

#include <ion/base.h>

VOID RtlFillMemory(VOID* Memory, UCHAR Value, SIZE_T Size);
VOID RtlCopyMemory(VOID* Destination, const VOID* Source, SIZE_T Size);
INT RtlCompareString(CHAR* String1, CHAR* String2, SIZE_T Size);
BOOLEAN RtlStringEquals(CHAR* String1, CHAR* String2);

FORCE_INLINE VOID RtlZeroMemory(VOID* Memory, SIZE_T Size) {
    return RtlFillMemory(Memory, 0, Size);
}

void *memset(void *s, int ch, size_t n);

#endif
