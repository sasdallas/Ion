/**
 * @file ionkrnl/io/packet.c
 * @brief IRP manager
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
#include <ion/ke/mm.h>

IONAPI IRP *IoAllocateIrp(UCHAR StackSize, BOOLEAN ChargeQuota) {
    SIZE_T IrpSize = (StackSize * sizeof(IO_STACK_LOCATION)) + sizeof(IRP);
    IRP *Irp = MmAllocateHeap(MmPagedHeap, IrpSize);
    if (Irp == NULL) {
        return NULL;
    }

    RtlZeroMemory(Irp, IrpSize);
    Irp->CurrentLocation = StackSize+1;
    Irp->StackCount = StackSize;
    Irp->CurrentStackLocation = &(((IO_STACK_LOCATION*)(Irp+1))[StackSize]);
    Irp->IoStatus.Status = STATUS_PENDING;
    return Irp;
}

IONAPI VOID IoCompleteRequest(IRP *Irp) {
    DbgPrint("IoCompleteRequest Irp=%p\n", Irp);

    IO_STACK_LOCATION *Stack = IoGetCurrentIrpStackLocation(Irp);
    while (Irp->CurrentLocation <= Irp->StackCount) {
        Irp->CurrentLocation++;

        if (Stack->Completion.CompletionRoutine != NULL) {
            // TODO: Cancelling IRPs
            BOOLEAN ShouldInvoke = FALSE;
            if (OS_SUCCESS(Irp->IoStatus.Status) && (Stack->Control & STACK_INVOKE_ON_SUCCESS)) {
                ShouldInvoke = TRUE;    
            } else if (OS_ERROR(Irp->IoStatus.Status) && (Stack->Control & STACK_INVOKE_ON_ERROR)) {
                ShouldInvoke = TRUE;
            }

            if (ShouldInvoke == TRUE) {
                OS_STATUS Status = Stack->Completion.CompletionRoutine(Stack->DeviceObject, Irp, Stack->Completion.Context);
                if (!OS_SUCCESS(Status)) {
                    // TODO: Implement STATUS_CONTINUE_COMPLETION/STATUS_MORE_PROCESSING_REQUIRED
                    DbgPrint("CompletionRoutine failed with status 0x%08x\n", Status);
                }
            }
        }

        Stack++;
    }

    // TODO: Make this an APC
    if (!(Irp->Flags & IRP_DEFER_FREEING)) {
        if (Irp->UserIoStatus != NULL) {
            RtlCopyMemory(Irp->UserIoStatus, &Irp->IoStatus, sizeof(IO_STATUS_BLOCK));
        }

        IoFreeIrp(Irp);
    }
}

/**
 * Frees an IRP 
 */
IONAPI VOID IoFreeIrp(IRP *Irp) {
    DbgPrint("IoFreeIrp Irp=%p\n", Irp);
    MmFreeHeap(Irp);
}
