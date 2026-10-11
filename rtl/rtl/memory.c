/**
 * @file rtl/rtl/memory.c
 * @brief Memory-management (RtlZeroMemory and friends)
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

VOID RtlFillMemory(VOID* Fill, UCHAR Value, SIZE_T Size) {
    UCHAR* Memory = (UCHAR*)Fill;
    while (Size--) {
        *Memory++ = Value;
    }
}
