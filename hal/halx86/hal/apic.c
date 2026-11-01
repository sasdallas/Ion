/**
 * @file hal/halx86/hal/apic.c
 * @brief APIC integration
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include "hal.h"
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>
#include <ion/ke/hal.h>
#include <ion/x86_64/port.h>
#include <uacpi/uacpi.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>

/* MADT */
static struct acpi_madt *HalpAcpiMadt;

/* Local APIC */
static VOID *HalpLocalApicIo;

#define APIC_EOI                0xB0
#define APIC_SPURIOUS_VECTOR    0xF0
#define APIC_LVT_TIMER          0x320
#define APIC_TIMER_INITIAL      0x380
#define APIC_TIMER_CURRENT      0x390
#define APIC_TIMER_DIVIDE       0x3E0
#define APIC_TIMER_VECTOR       0x40
#define APIC_SPURIOUS_IRQ       0xFE

FORCE_INLINE VOID HalpWriteLocalApic(UINT16 Register, UINT32 Value) {
    volatile UINT32* Pointer = (typeof(Pointer))((UINT_PTR)HalpLocalApicIo + Register);
    *Pointer = Value;
}

FORCE_INLINE UINT32 HalpReadLocalApic(UINT16 Register) {
    volatile UINT32* Pointer = (typeof(Pointer))((UINT_PTR)HalpLocalApicIo + Register);
    return *Pointer;
}

// TODO something useful with this information
static VOID HalpProcessMadtEntry(struct acpi_entry_hdr *Entry) {
    if (Entry->type == ACPI_MADT_ENTRY_TYPE_IOAPIC) {
        struct acpi_madt_ioapic *Pic = (typeof(Pic))Entry;
        DbgPrint("MADT: I/O APIC @ %p GsiBase 0x%08x Id 0x%02x\n", Pic->address, Pic->gsi_base, Pic->id);
    } else if (Entry->type == ACPI_MADT_ENTRY_TYPE_LAPIC) {
        struct acpi_madt_lapic *Pic = (typeof(Pic))Entry;
        DbgPrint("MADT: Local APIC @ Id 0x%02x Uid 0x%02x Flags 0x%08x\n", Pic->id, Pic->uid, Pic->flags);
    } else if (Entry->type == ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE) {
        struct acpi_madt_interrupt_source_override *Int = (typeof(Int))Entry;
        DbgPrint("MADT: Interrupt Source Override @ Bus 0x%02x Source 0x%02x Gsi 0x%02x Flags 0x%08x\n", Int->bus, Int->source, Int->gsi, Int->flags);
    } else if (Entry->type == ACPI_MADT_ENTRY_TYPE_LAPIC_NMI) {
        struct acpi_madt_lapic_nmi *Nmi = (typeof(Nmi))Entry;
        DbgPrint("MADT: Local APIC NMI @ Lint 0x%02x Uid 0x%02x Flags 0x%08x\n", Nmi->lint, Nmi->uid, Nmi->flags);
    } else {
        DbgPrint("HalpProcessMadtEntry: Unsupported APIC entry type %d\n", Entry->type);
    }
} 

static VOID HalpApicTimerInterrupt(TRAP_FRAME *TrapFrame) {
    HalpWriteLocalApic(APIC_EOI, 0);
    HalTimerInterrupt();
}

static VOID HalpApicSpuriousInterrupt(TRAP_FRAME *TrapFrame) {
    return;
}

static OS_STATUS HalpInitializeApicTimer() {
    OS_STATUS Status = KeSetInterruptHandler(APIC_TIMER_VECTOR, HalpApicTimerInterrupt);
    if (!OS_SUCCESS(Status)) return Status;

    Status = KeSetInterruptHandler(APIC_SPURIOUS_IRQ, HalpApicSpuriousInterrupt);
    if (!OS_SUCCESS(Status)) return Status;

    HalpWriteLocalApic(APIC_SPURIOUS_VECTOR, 0x100 | APIC_SPURIOUS_IRQ);
    HalpWriteLocalApic(APIC_LVT_TIMER, 0x10000 | APIC_TIMER_VECTOR);
    HalpWriteLocalApic(APIC_TIMER_DIVIDE, 0x3);

    // !!! Calibrate the APIC through the PIT
    UINT8 Speaker = PortRead8(0x61);
    PortWrite8(0x61, Speaker & ~0x03);
    PortWrite8(0x43, 0xB0);
    PortWrite8(0x42, 11932 & 0xFF);
    PortWrite8(0x42, 11932 >> 8);
    HalpWriteLocalApic(APIC_TIMER_INITIAL, 0xFFFFFFFF);
    PortWrite8(0x61, (Speaker & ~0x02) | 0x01);

    UINT32 Timeout = 100000000;
    while (!(PortRead8(0x61) & 0x20) && --Timeout) {
        KeStallExecution();
    }

    UINT32 Ticks = 0xFFFFFFFF - HalpReadLocalApic(APIC_TIMER_CURRENT);
    PortWrite8(0x61, Speaker);
    HalpWriteLocalApic(APIC_TIMER_INITIAL, 0);

    HalpWriteLocalApic(APIC_LVT_TIMER, 0x20000 | APIC_TIMER_VECTOR);
    HalpWriteLocalApic(APIC_TIMER_INITIAL, Ticks);
    return STATUS_SUCCESS;
}

OS_STATUS HalApicInitialize() {
    uacpi_table Table;
    uacpi_status Status = uacpi_table_find_by_signature("APIC", &Table);
    if (Status != UACPI_STATUS_OK) {
        DbgPrint("APIC table not located");
        return STATUS_NOT_FOUND;    
    }

    HalpAcpiMadt = (struct acpi_madt*)Table.ptr;

    struct acpi_entry_hdr *MadtListStart = (struct acpi_entry_hdr*)((UINT_PTR)HalpAcpiMadt + sizeof(struct acpi_madt));
    struct acpi_entry_hdr *MadtListEnd = (struct acpi_entry_hdr*)((UINT_PTR)HalpAcpiMadt + HalpAcpiMadt->hdr.length);
    while (MadtListStart < MadtListEnd) {
        HalpProcessMadtEntry(MadtListStart);
        MadtListStart = (struct acpi_entry_hdr*)((UINT_PTR)MadtListStart + MadtListStart->length);
    }

    // Map the local APIC in
    UINT64 ApicBase = __readmsr(0x1B); // IA32_MSR_APICBASE
    if (!(ApicBase & (1 << 11)) || (ApicBase & (1 << 10))) return STATUS_INTERNAL_ERROR;
    UINT_PTR ApicPhys = ApicBase & ~0xFFFULL;
    HalpLocalApicIo = MmMapIoSpace(ApicPhys, PAGE_SIZE, MmCacheUncached);
    if (HalpLocalApicIo == NULL) {
        return STATUS_NO_MEMORY;
    }

    // Timer
    OS_STATUS OsStatus = HalpInitializeApicTimer();
    if (!OS_SUCCESS(OsStatus)) {
        return OsStatus;
    }

    return STATUS_SUCCESS;
}
