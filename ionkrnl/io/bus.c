/**
 * @file ionkrnl/io/bus.c
 * @brief I/O bus manager
 *  
 * Example object manager layout:
 *  /Devices
 *      /Beep           <- This is a device attached to the root bus
 *      /USB            <- This is a bus type
 *       /HC0           <- This is a bus attached to a bus type
 *         /USBDevice0  <- This is a device
 *      /ACPI           <- This is a bus 
 * 
 * Bus types are a helper overlayed on bus to make it organized better, and allow devices that don't
 * care about instancing rules to use their children (i.e., a USB keyboard driver wouldn't care if
 * the device was under an xHCI or EHCI controller)
 * 
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/io.h>
#include <ion/ke/ob.h>
#include <ion/ke/os.h>
#include <ion/ke/mm.h>

/* Root bus */
BUS_OBJECT *IoRootBus = NULL;

/* Object types */
static OBJECT_TYPE *IoBusTypeObjectType;
static OBJECT_TYPE *IoBusObjectType;

IONAPI OS_STATUS IoCreateBusType(
    BUS_OBJECT *Parent,
    DRIVER_OBJECT *Driver,
    const CHAR *TypeName,
    BUS_TYPE **BusType
)
{
    *BusType = NULL;

    // Create the bus type object
    HANDLE TargetDirectory = Parent ? Parent->DirectoryHandle : IoDeviceDirectory;
    OBJECT_ATTRIBUTES TypeAttributes = {
        .ObjectName = NULL,
        .RootDirectory = NULL,
        .Flags = OB_FLAG_KERNEL_OBJECT,
    };

    BUS_TYPE *Type;
    OS_STATUS Status = ObCreateObject(IoBusTypeObjectType, &TypeAttributes, sizeof(BUS_TYPE), (VOID**)&Type);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Fill out tha fields
    Type->TypeName = TypeName;
    Type->ParentBus = Parent;
    Type->Driver = Driver;
    Type->NrInstances = 0;
    Type->IsPending = FALSE;
    Type->DirectoryHandle = NULL;
    KeInitializePushLock(&Type->Lock);
    InitializeListHead(&Type->Instances);

    // Create the sub-directory in /Devices/
    OBJECT_ATTRIBUTES DirectoryAttributes = {
        .ObjectName = (CHAR*)TypeName,
        .RootDirectory = TargetDirectory,
        .Flags = OB_FLAG_KERNEL_OBJECT,
    };

    Status = OsCreateDirectoryObject(&Type->DirectoryHandle, GENERIC_ALL, &DirectoryAttributes);
    if (!OS_SUCCESS(Status)) {
        ObDereferenceObject(Type);
        return Status;
    }

    *BusType = Type;
    return STATUS_SUCCESS;
}

IONAPI OS_STATUS IoCreateBus(
    BUS_TYPE *Type,
    CHAR *Name,
    BUS_OBJECT **BusOutput
)
{
    OBJECT_ATTRIBUTES Attributes = { 0 };
    if (Type) {
        Attributes.ObjectName = Name;
        Attributes.RootDirectory = Type->DirectoryHandle;
    } else {
        Attributes.ObjectName = Name;
        Attributes.RootDirectory = IoDeviceDirectory;
    }

    // Create the object
    BUS_OBJECT *Bus;
    OS_STATUS Status = ObCreateObject(IoBusObjectType, &Attributes, sizeof(BUS_OBJECT), (VOID**)&Bus);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Prepare the bus
    InitializeListHead(&Bus->Devices);
    KeInitializePushLock(&Bus->Lock);
    Bus->NrDevices = 0;
    Bus->Type = Type;    
    Bus->InstanceName = Name;
    

    if (Name) {
        OBJECT_ATTRIBUTES DirectoryAttributes = {
            .ObjectName = Name,
            .Flags = 0,
            .RootDirectory = Type ? Type->DirectoryHandle : IoDeviceDirectory
        };

        Status = OsCreateDirectoryObject(&Bus->DirectoryHandle, GENERIC_ALL, &DirectoryAttributes);;
        if (!OS_SUCCESS(Status)) {
            ObDereferenceObject(Bus);
            return Status;
        }
    } else {
        // TODO should duplicate this handle
        Bus->DirectoryHandle = IoDeviceDirectory;
    }

    if (Type != NULL) {
        KeAcquirePushLockExclusive(&Type->Lock);
        InsertTailList(&Type->Instances, &Bus->Entry);
        Type->NrInstances++;
        KeReleasePushLockExclusive(&Type->Lock);
    }

    *BusOutput = Bus;
    return STATUS_SUCCESS;
}

IONAPI BUS_OBJECT *IoGetRootBus() {
    return IoRootBus;
}

OS_STATUS IopInitializeRootBus() {
    OBJECT_TYPE_ATTRIBUTES Attributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = NULL,
        .DumpProcedure = NULL,
        .ParseProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmNonpagedHeap
    };

    OS_STATUS Status = ObCreateObjectType("IoBusType", &Attributes, &IoBusTypeObjectType);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    Status = ObCreateObjectType("IoBus", &Attributes, &IoBusObjectType);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    Status = IoCreateBus(NULL, NULL, &IoRootBus);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

