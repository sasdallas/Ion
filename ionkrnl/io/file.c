/**
 * @file ionkrnl/io/file.c
 * @brief File device
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
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ke/os.h>

/* File object */
static OBJECT_TYPE *IopFileObjectType;

static VOID IopDestroyFile(VOID *Object) {
    FILE_OBJECT *FileObject = Object;
    if (FileObject->FileName) MmFreeHeap(FileObject->FileName);
}

/**
 * Handles parsing a device/file using the string views. Constructs an open
 * irp and sends it to the device.
 * 
 * TODO: ParseContext will be needed once this system's flags/bookkeeping is flushed out
 */
OS_STATUS IopParseDevice(VOID *ParseObject, VOID *ObjectType, OB_STRING_VIEW *CompleteName, OB_STRING_VIEW *RemainingName, VOID *Context, VOID **Object) {
    DEVICE_OBJECT *Device = ParseObject;

    // Create the file object
    OBJECT_ATTRIBUTES FileObjectAttributes = {
        .ObjectName = NULL,
        .Flags = 0, // TODO Attributes
        .RootDirectory = NULL,
    };

    FILE_OBJECT *FileObject;
    OS_STATUS Status = ObCreateObject(IopFileObjectType, &FileObjectAttributes, sizeof(FILE_OBJECT), (VOID**)&FileObject);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Initialize file object
    FileObject->DeviceObject = Device;
    FileObject->FileName = NULL;
    InitializeListHead(&FileObject->IrpList);
    KeInitializeSpinLock(&FileObject->IrpListLock);
    if (RemainingName->Length) {
        FileObject->FileName = MmAllocateHeap(MmPagedHeap, RemainingName->Length+1);
        if (!FileObject->FileName) {
            ObDereferenceObject(FileObject);
            return STATUS_NO_MEMORY;
        }

        RtlCopyMemory(FileObject->FileName, RemainingName->Buffer, RemainingName->Length);
        FileObject->FileName[RemainingName->Length] = 0;
    }

    // TODO Add FileObject->Flags and support it here
    FileObject->Flags = 0;

    // Create the IRP
    IRP *Irp = IoAllocateIrp(Device->IrpStackSize, TRUE);
    if (Irp == NULL) {
        ObDereferenceObject(FileObject);
        return STATUS_NO_MEMORY;
    }

    // We want to hold on to this IRP and free it later, so we can retrieve IoStatus
    Irp->Flags |= IRP_DEFER_FREEING;

    // Prepare IRP stack
    IO_STACK_LOCATION *Stack = IoGetNextIrpStackLocation(Irp);

    Stack->MajorFunction = IRP_MJ_CREATE;
    Stack->MinorFunction = 0;
    Stack->FileObject = FileObject;

    // Call the driver
    Status = IoCallDriver(Device, Irp);
    ASSERT(Status != STATUS_PENDING && "No support yet");
    ASSERT(Irp->CurrentLocation > Irp->StackCount && "Create IRP was not completed");
    OS_STATUS CompletionStatus = Irp->IoStatus.Status;
    IoFreeIrp(Irp);
    if (!OS_SUCCESS(Status)) {
        DbgPrint("IopParseDevice: IoCallDriver failed with status 0x%08x\n", Status);
        ObDereferenceObject(FileObject);
        return Status;
    }

    if (!OS_SUCCESS(CompletionStatus)) {
        ObDereferenceObject(FileObject);
        return CompletionStatus;
    }

    *Object = FileObject;
    return CompletionStatus;
}

/**
 * Creates the file type
 */
OS_STATUS IopInitializeFileManager() {
    OBJECT_TYPE_ATTRIBUTES TypeAttributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = IopDestroyFile,
        .DumpProcedure = NULL,
        .ParseProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmPagedHeap
    };

    return ObCreateObjectType("IoFile", &TypeAttributes, &IopFileObjectType);
}

/**
 * Creates/opens/whatevers a file according to Disposition
 */
IONAPI OS_STATUS IoCreateFile(
    HANDLE *FileHandle,
    ACCESS_MASK DesiredAccess,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    LONGLONG InitialAllocationSize,
    UCHAR Disposition,
    ULONG Options
)
{
    if (Disposition != FILE_OPEN) {
        KeBugCheckWithMessage("Dispositions other than FILE_OPEN are not supported at the moment");
    }

    HANDLE Handle;
    OS_STATUS Status = ObOpenObjectByName(
        ObjectAttributes,
        NULL,
        KernelMode, // TODO
        DesiredAccess,
        NULL,
        &Handle
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    *FileHandle = Handle;
    return STATUS_SUCCESS;
}

/**
 * Standard system service
 */
IONAPI OS_STATUS OsOpenFile(
    OUT HANDLE *FileHandle,
    ACCESS_MASK DesiredAccess,
    OBJECT_ATTRIBUTES *ObjectAttributes,
    ULONG OpenOptions
)
{
    return IoCreateFile(
        FileHandle,
        DesiredAccess,
        ObjectAttributes,
        0,
        FILE_OPEN,
        OpenOptions
    );
}

/**
 * Standard system service
 */
IONAPI OS_STATUS OsDeviceIoControlFile(
    HANDLE FileHandle,
    HANDLE Event,
    IO_STATUS_BLOCK *IoStatus,
    ULONG IoControlCode,
    VOID *InputBuffer,
    ULONG InputBufferLength,
    VOID *OutputBuffer,
    ULONG OutputBufferLength
)
{
    if (Event != NULL) {
        KeBugCheckWithMessage("OsDeviceIoControlFile(): event not impl");
    }

    FILE_OBJECT *FileObject;
    OS_STATUS Status = ObReferenceObjectByHandle(FileHandle, IopFileObjectType, KernelMode, (VOID**)&FileObject);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Create the IRP
    IRP *Irp = IoAllocateIrp(FileObject->DeviceObject->IrpStackSize, TRUE);
    if (Irp == NULL) {
        ObDereferenceObject(FileObject);
        return STATUS_NO_MEMORY;
    }

    Irp->UserIoStatus = IoStatus;
    Irp->Flags |= IRP_DEFER_FREEING;

    IO_STACK_LOCATION *Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    Stack->MinorFunction = 0;
    Stack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    Stack->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    Stack->Parameters.DeviceIoControl.IoInputBuffer = InputBuffer;
    Stack->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    Stack->Parameters.DeviceIoControl.IoOutputBuffer = OutputBuffer;
    Stack->FileObject = FileObject;

    Status = IoCallDriver(FileObject->DeviceObject, Irp);
    ASSERT(Status != STATUS_PENDING && "Asynchronous device control is not supported yet");
    ASSERT(Irp->CurrentLocation > Irp->StackCount && "Device control IRP was not completed");
    if (IoStatus) *IoStatus = Irp->IoStatus;
    IoFreeIrp(Irp);
    ObDereferenceObject(FileObject);
    return Status;
}

/**
 * Standard system service
 */
IONAPI OS_STATUS OsClose(HANDLE Handle) {
    return ObCloseHandle(Handle);
}
