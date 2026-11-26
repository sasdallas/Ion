/**
 * @file include/ion/ke/io.h
 * @brief Ion's I/O subsystem
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_IO_H
#define _ION_KE_IO_H 1

#include <ion/base.h>

#include <ion/ke/io/driver.h>
#include <ion/ke/io/bus.h>
#include <ion/ke/io/device.h>

#include <ion/ke/ob/object.h>

extern OBJECT_TYPE *IoDriverObjectType;
extern HANDLE IoDriversDirectory, IoDeviceDirectory;

IONAPI OS_STATUS IoCreateBusType(
    BUS_OBJECT *Parent,
    DRIVER_OBJECT *Driver,
    const CHAR *TypeName,
    BUS_TYPE **BusType
);

IONAPI OS_STATUS IoCreateBus(
    BUS_TYPE *Type,
    CHAR *Name,
    BUS_OBJECT **Bus
);

IONAPI BUS_OBJECT *IoGetRootBus();

IONAPI OS_STATUS IoCreateDevice(
    DRIVER_OBJECT *Driver,
    BUS_OBJECT *Bus,
    CHAR *DeviceName,
    DEVICE_TYPE DeviceType,
    ULONG Flags,
    DEVICE_OBJECT **DeviceObject
);

OS_STATUS IoInitialize();

#endif
