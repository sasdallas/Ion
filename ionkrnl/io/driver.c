/**
 * @file ionkrnl/io/driver.c
 * @brief I/O driver manager
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

OBJECT_TYPE *IoDriverObjectType = NULL;
HANDLE IoDriversDirectory = NULL;

OS_STATUS IopInitializeDrivers() {
    OBJECT_TYPE_ATTRIBUTES Attributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DumpProcedure = NULL,
        .ParseProcedure = NULL,
        .DestroyProcedure = NULL, // TODO
        .TargetHeap = MmNonpagedHeap,
    };

    OS_STATUS Status = ObCreateObjectType("IoDriver", &Attributes, &IoDriverObjectType);
    if (!OS_SUCCESS(Status)) {
        DbgPrintError("ObCreateObjectType", Status);
        return Status;
    }

    // Create the drivers directory
    OBJECT_ATTRIBUTES DirectoryAttributes = {
        .ObjectName = "/Drivers",
        .RootDirectory = NULL,
        .Flags = OB_FLAG_KERNEL_OBJECT | OB_FLAG_PERMANENT_OBJECT,
    };

    Status = OsCreateDirectoryObject(&IoDriversDirectory, GENERIC_READ | GENERIC_WRITE, &DirectoryAttributes);
    if (!OS_SUCCESS(Status)) {
        DbgPrintError("OsCreateDirectoryObject", Status);
        return Status;
    }

    return LdrLoadBootDrivers();
}
