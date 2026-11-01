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
}
