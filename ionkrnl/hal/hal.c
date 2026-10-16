/**
 * @file ionkrnl/hal/hal.c
 * @brief HAL linkage
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/hal.h>
#include <ion/rtl.h>
#include <ion/hal.h>
#include <ion/ion.h>

/* Primary vtable */
static HAL_VTABLE *HalpVtable = NULL;

IONAPI VOID HalSetVtable(HAL_VTABLE *Vtable) {
    HalpVtable = Vtable;
}

IONHALAPI VOID HalPrintString(const CHAR* String) {
    if (HalpVtable == NULL) return;
    return HalpVtable->PrintString(String);
}

IONHALAPI OS_STATUS HalInitSystemConsole() {
    return HalpVtable->InitSystemConsole();
}

IONHALAPI VOID HalConsolePrintf(const CHAR* Format, ...) {
    CHAR Buffer[512];
    va_list ap;
    va_start(ap, Format);
    RtlFormatStringVa(Buffer, 512, Format, ap);
    va_end(ap);

    HalPrintString(Buffer);
}
