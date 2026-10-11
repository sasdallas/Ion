/**
 * @file rtl/rtl/dbg.c
 * @brief RTL debug functions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

#ifndef KERNEL

OS_STATUS DbgInit() {
    // DbgInit NYI
    return STATUS_SUCCESS;
}

VOID DbgPrintString(const CHAR* String) {
    // DbgPrintString NYI
}

#endif

VOID DbgPrintMsg(const CHAR* Message, ...) {
    char Buffer[512];

    va_list Ap;
    va_start(Ap, Message);
    RtlFormatStringVa(Buffer, 512, Message, Ap);
    va_end(Ap);

    DbgPrintString(Buffer);
}
