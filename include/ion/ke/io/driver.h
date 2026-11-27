/**
 * @file include/ion/ke/io/driver.h
 * @brief I/O Driver
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_IO_DRIVER_H
#define _ION_KE_IO_DRIVER_H 1

#include <ion/base.h>
#include <ion/ke/ldr.h>
#include <ion/ke/io/packet.h>

struct _DRIVER_OBJECT;

typedef OS_STATUS (*DRIVER_ENTRY)(struct _DRIVER_OBJECT *DriverObject);
typedef OS_STATUS (*DRIVER_UNLOAD)(struct _DRIVER_OBJECT *DriverObject);

typedef struct _DRIVER_OBJECT {
    const CHAR *DriverName;
    LOADED_DLL *Dll;

    LIST_ENTRY Devices;
    SIZE_T NrDevices;

    DRIVER_ENTRY DriverEntry;
    DRIVER_UNLOAD DriverUnload;

    IO_DISPATCH_ROUTINE MajorFunction[IRP_MJ_NUMBER];
} DRIVER_OBJECT;

#endif
