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
#include <ion/ke/io/packet.h>
#include <ion/ke/io/file.h>

#include <ion/ke/ob/object.h>

extern OBJECT_TYPE *IoDriverObjectType;
extern HANDLE IoDriversDirectory, IoDeviceDirectory;

/* Bus */
IONAPI OS_STATUS IoCreateBusType(BUS_OBJECT *Parent, DRIVER_OBJECT *Driver, const CHAR *TypeName, BUS_TYPE **BusType);
IONAPI OS_STATUS IoCreateBus(BUS_TYPE *Type, CHAR *Name, BUS_OBJECT **Bus);
IONAPI BUS_OBJECT *IoGetRootBus();

/* Device */
IONAPI OS_STATUS IoCreateDevice(
    DRIVER_OBJECT *Driver,
    BUS_OBJECT *Bus,
    CHAR *DeviceName,
    DEVICE_TYPE DeviceType,
    ULONG Flags,
    DEVICE_OBJECT **DeviceObject
);

IONAPI DEVICE_OBJECT *IoAttachDeviceToDeviceStack(DEVICE_OBJECT *SourceDevice, DEVICE_OBJECT *TargetDevice);
IONAPI OS_STATUS IoGetDeviceObjectPointer(CHAR *ObjectName, ACCESS_MASK DesiredAccess, FILE_OBJECT **FileObject, DEVICE_OBJECT **DeviceObject);
IONAPI OS_STATUS IoCallDriver(DEVICE_OBJECT *Device, IRP *Irp);

/* File */
IONAPI OS_STATUS IoCreateFile(
    HANDLE *FileHandle,
    ACCESS_MASK DesiredAccess,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    LONGLONG InitialAllocationSize,
    UCHAR Disposition,
    ULONG Options
);

/* IRP */
IONAPI IRP *IoAllocateIrp(UCHAR StackSize, BOOLEAN ChargeQuota);
IONAPI VOID IoCompleteRequest(IRP *Irp);
IONAPI VOID IoFreeIrp(IRP *Irp);

/* Private */
OS_STATUS IopParseDevice(VOID *ParseObject, VOID *ObjectType, OB_STRING_VIEW *CompleteName, OB_STRING_VIEW *RemainingName, VOID *Context, VOID **Object);
IONAPI OS_STATUS IopInvalidDeviceRequest(DEVICE_OBJECT *DeviceObject, IRP *Irp);

/* Misc. */
OS_STATUS IoInitialize();

#endif
