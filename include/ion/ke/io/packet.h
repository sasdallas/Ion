/**
 * @file include/ion/ke/io/packet.h
 * @brief Packet structure
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_IO_PACKET_H
#define _ION_KE_IO_PACKET_H 1

#include <ion/base.h>
#include <ion/list.h>
#include <ion/rtl/assert.h>
#include <ion/ke/io/file.h>

#define IRP_MJ_CREATE             0x00
#define IRP_MJ_CLOSE            0x01
#define IRP_MJ_DEVICE_CONTROL   0x02
#define IRP_MJ_NUMBER           3

#define IRP_DEFER_FREEING       0x01

// Stack stuff
#define STACK_INVOKE_ON_SUCCESS    0x01 // invoke CompletionRoutine on success
#define STACK_INVOKE_ON_ERROR      0x02 // invoke CompletionRoutine on error
#define STACK_INVOKE_ON_CANCEL     0x04 // invoke CompletionRoutine on cancel

struct _IRP;
struct _DEVICE_OBJECT;
typedef OS_STATUS (*IO_DISPATCH_ROUTINE)(struct _DEVICE_OBJECT *DeviceObject, struct _IRP *Irp);
typedef OS_STATUS (*IO_COMPLETION_ROUTINE)(struct _DEVICE_OBJECT *DeviceObject, struct _IRP *Irp, VOID *Context);

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    ULONG Control;

    union {
        struct {
            ULONG IoControlCode;
            VOID *IoInputBuffer; // TODO: maybe larp Windows even more with UserBuffer and AssociatedIrp.SystemBuffer?
            ULONG InputBufferLength;
            VOID *IoOutputBuffer;
            ULONG OutputBufferLength;
        } DeviceIoControl;
    } Parameters;

    struct {
        IO_COMPLETION_ROUTINE CompletionRoutine;
        VOID *Context;
    } Completion;

    struct _DEVICE_OBJECT *DeviceObject;
    FILE_OBJECT *FileObject;
} IO_STACK_LOCATION;

typedef struct _IO_STATUS_BLOCK {
    OS_STATUS Status;
    UINT_PTR CompletionDependent;
} IO_STATUS_BLOCK;

typedef struct _IRP {
    ULONG Flags;
    LIST_ENTRY ThreadListEntry;

    IO_STATUS_BLOCK IoStatus;
    IO_STATUS_BLOCK *UserIoStatus;

    UCHAR StackCount;
    CHAR CurrentLocation;
    BOOLEAN Cancel;

    IO_STACK_LOCATION *CurrentStackLocation;
} IRP;

FORCE_INLINE IO_STACK_LOCATION *IoGetNextIrpStackLocation(IRP *Irp) {
    ASSERT(Irp->CurrentLocation > 0);

    return Irp->CurrentStackLocation - 1;
}

FORCE_INLINE IO_STACK_LOCATION *IoGetCurrentIrpStackLocation(IRP *Irp) {
    return Irp->CurrentStackLocation;
}

#endif
