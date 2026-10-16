/**
 * @file hal/halx86/hal/init.c
 * @brief Halx86 initialization code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/hal.h>
#include <ion/ke/hal.h>
#include <ion/ion.h>

VOID Halx86PrintString(const CHAR *String);
OS_STATUS Halx86InitSystemConsole();

HAL_VTABLE Halx86Vtable = {
    .InitSystemConsole = Halx86InitSystemConsole,
    .PrintString = Halx86PrintString
};

__declspec(dllexport) OS_STATUS HalInitSystem() {
    DbgPrint("HalInitSystem\n");

    HalSetVtable(&Halx86Vtable);

    return STATUS_SUCCESS;
}
