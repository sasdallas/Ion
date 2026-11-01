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

#include "hal.h"
#include <ion/hal.h>
#include <ion/ke/hal.h>
#include <ion/ion.h>
#include <uacpi/uacpi.h>

VOID Halx86PrintString(const CHAR *String);
OS_STATUS Halx86InitSystemConsole();
OS_STATUS Halx86InitializeHardware();

HAL_VTABLE Halx86Vtable = {
    .InitializeHardware = Halx86InitializeHardware,
    .PrintString = Halx86PrintString
};

OS_STATUS Halx86InitializeHardware() {
    OS_STATUS Status = HalAcpiInitialize();
    if (!OS_SUCCESS(Status)) {
        return Status;
    };

    Status = HalApicInitialize();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

__declspec(dllexport) OS_STATUS HalInitSystem() {
    DbgPrint("x86 HAL begin initialization...\n");
    HalSetVtable(&Halx86Vtable);

    OS_STATUS Status = Halx86InitSystemConsole();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}
