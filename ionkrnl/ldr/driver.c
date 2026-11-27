/**
 * @file ionkrnl/ldr/driver.c
 * @brief Driver loader
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ldr.h>
#include <ion/ke/io.h>
#include <ion/ke/ob.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>

/* Boot drivers */
LIST_ENTRY LdrBootDriverList;

/* Smal structure */
typedef struct _LOADED_DRIVER {
    LIST_ENTRY Entry;
    LOADED_DLL *Dll;
    DRIVER_OBJECT *Driver;
    HANDLE Handle;
} LOADED_DRIVER;

static OS_STATUS LdrpMapBootDriver(LOADER_MODULE *Module) {
    LOADED_DRIVER *LoadedDriver = MmAllocateHeap(MmPagedHeap, sizeof(LOADED_DRIVER));
    if (LoadedDriver == NULL) {
        return STATUS_NO_MEMORY;
    }

    LOADED_DLL *Dll;
    OS_STATUS Status = LdrMapBootModule(Module, &Dll);
    if (!OS_SUCCESS(Status)) {
        DbgPrint("Error loading boot module %s: %s (0x%08x)\n", Module->Path, RtlStatusToString(Status), Status);
        MmFreeHeap(LoadedDriver);
        return Status;
    }

    LoadedDriver->Dll = Dll;
    LoadedDriver->Driver = NULL;
    LoadedDriver->Handle = NULL;
    InsertTailList(&LdrBootDriverList, &LoadedDriver->Entry);

    return STATUS_SUCCESS;
}

OS_STATUS LdrpLoadBootDriver(LOADED_DRIVER *LoadedDriver) {
    LOADED_DLL *Dll = LoadedDriver->Dll;
    OS_STATUS Status;

    DbgPrint("Loading boot driver: %s\n", LoadedDriver->Dll->Name);

    // TODO: No stack storage
    CHAR Name[256];
    RtlCopyStringN(Name, Dll->Name, 256);
    CHAR *PName = Name;
    while (*PName != '.') PName++;
    *PName = 0;

    // Create the driver object
    OBJECT_ATTRIBUTES Attributes = {
        .ObjectName = Name,
        .RootDirectory = IoDriversDirectory, 
        .Flags = OB_FLAG_KERNEL_OBJECT,
    };

    DRIVER_OBJECT *Driver;
    Status = ObCreateObject(IoDriverObjectType, &Attributes, sizeof(DRIVER_OBJECT), (VOID**)&Driver);
    if (!OS_SUCCESS(Status)) {
        DbgPrintError("ObCreateObject", Status);
        return Status;
    }

    // Prepare driver object
    // TODO: Should move this junk logic to io/driver.c
    InitializeListHead(&Driver->Devices);
    Driver->NrDevices = 0;
    Driver->DriverName = Dll->Name;
    Driver->Dll = Dll;
    Driver->DriverEntry = (DRIVER_ENTRY)LdrGetSymbol(Dll, "DriverEntry");
    Driver->DriverUnload = (DRIVER_ENTRY)LdrGetSymbol(Dll, "DriverUnload");
    if (Driver->DriverEntry == NULL) {
        DbgPrint("Entrypoint not found in %s\n", Dll->Name);
        ObDereferenceObject(Driver);
        return STATUS_ENTRYPOINT_NOT_FOUND;
    }

    for (INT i = 0; i < IRP_MJ_NUMBER; i++) {
        Driver->MajorFunction[i] = IopInvalidDeviceRequest;
    }

    // We keep a pointer reference for slightly longer, and ObInsertObject consumes the one
    // reference this device has
    ObReferenceObject(Driver);

    // Create a HANDLE on the driver
    HANDLE DriverHandle;
    Status = ObInsertObject(Driver, NULL, &DriverHandle);
    if (!OS_SUCCESS(Status)) {
        DbgPrintError("ObInsertObject", Status);
        ObDereferenceObject(Driver);
        return Status;
    }

    // Finished loading driver
    LoadedDriver->Driver = Driver;
    LoadedDriver->Handle = DriverHandle;

    // Execute!
    Status = Driver->DriverEntry(Driver);
    if (!OS_SUCCESS(Status)) {
        // Not critical
        DbgPrint("\033[0;33mDriver %s failed initialization:\033[0m %s (0x%08x)\n", Driver->DriverName, RtlStatusToString(Status), Status);
    }

    ObDereferenceObject(Driver);
    return STATUS_SUCCESS;
}

OS_STATUS LdrLoadBootDrivers() {
    InitializeListHead(&LdrBootDriverList);

    LOADER_PARAMETER_BLOCK *Lpb = KeGetLoaderParameterBlock();
    for (SIZE_T i = 1; i < Lpb->NrLoadedModules; i++) {
        LOADER_MODULE *Module = &Lpb->LoadedModules[i];
        if (Module->Cmdline == NULL || !RtlStringEquals(Module->Cmdline, "/bootdrv")) {
            continue;
        }

        OS_STATUS Status = LdrpMapBootDriver(Module);
        if (!OS_SUCCESS(Status)) {
            return Status;
        }
    }

    // resolve all the imports first
    for (LIST_ENTRY *Entry = LdrBootDriverList.Flink; Entry != &LdrBootDriverList; Entry = Entry->Flink) {
        LOADED_DRIVER *LoadedDriver = CONTAINING_RECORD(Entry, LOADED_DRIVER, Entry);
        OS_STATUS Status = LdrResolveDllImports(LoadedDriver->Dll);
        if (!OS_SUCCESS(Status)) {
            DbgPrint("Resolving DLL imports on %s failed: %s (0x%08x)\n", LoadedDriver->Dll->Name, RtlStatusToString(Status), Status);
            return Status;
        }
    }

    // now create each boot driver
    for (LIST_ENTRY *Entry = LdrBootDriverList.Flink; Entry != &LdrBootDriverList; Entry = Entry->Flink) {
        LOADED_DRIVER *LoadedDriver = CONTAINING_RECORD(Entry, LOADED_DRIVER, Entry);
        OS_STATUS Status = LdrpLoadBootDriver(LoadedDriver);
        if (!OS_SUCCESS(Status)) {
            return Status;
        }
    }

    return STATUS_SUCCESS;
}
