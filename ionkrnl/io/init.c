/**
 * @file ionkrnl/io/init.c
 * @brief I/O initialization
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/io.h>
#include <ion/ke/ke.h>
#include <ion/ke/ob.h>
#include <ion/ke/os.h>

/* Device directory */
HANDLE IoDeviceDirectory = NULL;

extern OS_STATUS IopInitializeDrivers();
extern OS_STATUS IopInitializeRootBus();
extern OS_STATUS IopInitializeDeviceManager();

OS_STATUS IoInitialize() {
    // Create the device directory
    OBJECT_ATTRIBUTES DeviceAttributes = {
        .ObjectName = "Devices",
        .RootDirectory = NULL,
        .Flags = OB_FLAG_KERNEL_OBJECT | OB_FLAG_PERMANENT_OBJECT,
    };

    OS_STATUS Status = OsCreateDirectoryObject(&IoDeviceDirectory, GENERIC_ALL, &DeviceAttributes);
    if (!OS_SUCCESS(Status)) {
        DbgPrintError("OsCreateDirectoryObject", Status);
        return Status;
    }

    // Initialize root bus
    Status = IopInitializeRootBus();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Initialize devmgr
    Status = IopInitializeDeviceManager();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Initialize boot-time drivers
    Status = IopInitializeDrivers();
    if (!OS_SUCCESS(Status)) {
        return Status;
    }


    return STATUS_SUCCESS;
}
