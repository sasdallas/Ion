/**
 * @file ionkrnl/ke/irql.c
 * @brief IRQL management
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ion.h>
#include <ion/ke/ke.h>

/*
 * Expected to be called whenever IRQL drops below a certain level.
 * Only IRQLs pending below NewIrql can be executed
 * Should be called with interrupts enabled.
 * 
 * KiDispatchPendingSoftwareInterrupt's architecture is inspired by Boron + MINTIA2
*/
VOID KiDispatchPendingSoftwareInterrupt(KIRQL NewIrql) {
    KCPU *Cpu = KeGetCpu();
    BOOLEAN Restore = KeSetInterruptState(FALSE);

    while (1) {
        INT Pending = Cpu->PendingSoftInterrupt & (UCHAR)(0xFF << NewIrql);
        if (Pending == 0) {
            break;
        }

        // There are really only two software interrupts used in Ion,
        // those being APCs and DPCs, so there's no real need to design a whole queue system 
        if (Pending & KeIrqlPending(DISPATCH_LEVEL)) {
            KiDpcInterrupt();
        } else {
            KeBugCheckWithMessage("KiDispatchPendingSoftwareInterrupts(): Bad pending set 0x%x", Pending);
        }
    }

    // Restore IRQL, some of the software interrupt functions may have set it to something else entirely.
    Cpu->Irql = NewIrql;

    KeSetInterruptState(Restore);
}

/*
 * Acknowledge a software interrupt has occurred
 * Called from either KiDpcInterrupt or KiApcInterrupt
 */
VOID KiAcknowledgeSoftwareInterrupt(KIRQL Irql) {
    InterlockedAnd(&KeGetCpu()->PendingSoftInterrupt, ~KeIrqlPending(Irql));
}

/*
 * Marks a pending software interrupt at Irql on the CPU
 * The software interrupt will be executed asyncronously when Cpu->Irql
 * is less than the requested one.
 */
IONAPI VOID KeRequestSoftwareInterrupt(KIRQL Irql) {
    if (Irql != DISPATCH_LEVEL) {
        KeBugCheckWithMessage("Unsupported software interrupt IRQL 0x%x", Irql);
    }

    InterlockedOr(&KeGetCpu()->PendingSoftInterrupt, KeIrqlPending(Irql));
}

/*
 * Raise IRQL to NewIrql and save old Irql
 */
IONAPI VOID KeRaiseIrql(KIRQL NewIrql, KIRQL *OldIrql) {
    BOOLEAN Restore = KeSetInterruptState(FALSE);

    KCPU *Cpu = KeGetCpu();
    *OldIrql = Cpu->Irql;

    if (Cpu->Irql == NewIrql) {
        goto _Exit;
    } else if (Cpu->Irql > NewIrql) {
        KeBugCheckWithMessage("IRQL not less or equal (NewIrql=0x%x Cpu->Irql=0x%x)", NewIrql, Cpu->Irql);
    }

    Cpu->Irql = NewIrql;

    KeHardwareUpdateIrql(NewIrql);

_Exit:
    KeSetInterruptState(Restore);
}

/*
 * Lower IRQL to Irql
 * Will throw an exception if the current CPU's IRQL < Irql
 * 
 * If interrupts are enabled, KiDispatchPendingSoftwareInterrupt will be called
 * to handle software interrupts < Irql
 */
IONAPI VOID KeLowerIrql(KIRQL Irql) {
    BOOLEAN Restore = KeSetInterruptState(FALSE);

    KCPU *Cpu = KeGetCpu();
    if (Cpu->Irql == Irql) {
        goto _Exit;  
    } else if (Cpu->Irql < Irql) {
        KeBugCheckWithMessage("IRQL not greater than or equal (Irql=0x%x Cpu->Irql=0x%x)", Irql, Cpu->Irql);
    }

    Cpu->Irql = Irql;
    KeHardwareUpdateIrql(Irql);

_Exit:
    KeSetInterruptState(Restore);
 
    if (Restore) {
        KiDispatchPendingSoftwareInterrupt(Irql);
    }
}
