/**
 * @file hal/halx86/hal/acpi.c
 * @brief HAL ACPI management
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include "hal.h"
#include <ion/ion.h>
#include <uacpi/kernel_api.h>
#include <uacpi/status.h>
#include <uacpi/types.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <uacpi/sleep.h>
#include <uacpi/event.h>

OS_STATUS HalAcpiInitialize() {
    uacpi_status status = uacpi_initialize(0);
    if (status != UACPI_STATUS_OK) {
        DbgPrint("uacpi_initialize failed with error code 0x%x\n", status);
        return STATUS_INTERNAL_ERROR;
    }

    // TODO Uacpi namespace
    DbgPrint("uACPI namespace init todo\n");

    return STATUS_SUCCESS;
}
