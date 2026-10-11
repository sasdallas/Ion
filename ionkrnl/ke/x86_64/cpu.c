/**
 * @file ionkrnl/ke/x86_64/cpu.c
 * @brief CPU setup code
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

#define IDT_DESC_BIT32          0x0E
#define IDT_DESC_PRESENT        0x80

typedef struct _KIDTENTRY {
    UINT16 BaseLo;
    UINT16 Selector;
    UINT8 Ist;
    UINT8 Flags;
    UINT16 BaseMid;
    UINT32 BaseHi;
    UINT32 Reserved;
} PACKED KIDTENTRY;

typedef struct _KIDT {
    KIDTENTRY Entries[256];
} KIDT;

/* Kernel exception handlers */
extern VOID KiDivisionException(VOID);
extern VOID KiDebugException(VOID);
extern VOID KiNMIException(VOID);
extern VOID KiBreakpointException(VOID);
extern VOID KiOverflowException(VOID);
extern VOID KiBoundException(VOID);
extern VOID KiInvalidOpcodeException(VOID);
extern VOID KiNoFPUException(VOID);
extern VOID KiDoubleFaultException(VOID);
extern VOID KiCoprocessorSegmentException(VOID);
extern VOID KiInvalidTSSException(VOID);
extern VOID KiSegmentNotPresentException(VOID);
extern VOID KiStackSegmentException(VOID);
extern VOID KiGeneralProtectionException(VOID);
extern VOID KiPageFaultException(VOID);
extern VOID KiReservedException(VOID);
extern VOID KiFloatingPointException(VOID);
extern VOID KiAlignmentCheck(VOID);
extern VOID KiMachineCheck(VOID);
extern VOID KiSIMDFloatingPointException(VOID);
extern VOID KiVirtualizationException(VOID);
extern VOID KiControlProtectionException(VOID);
extern VOID KiHypervisorInjectionException(VOID);
extern VOID KiVMMCommunicationException(VOID);
extern VOID KiSecurityException(VOID);
extern VOID KiReserved2Exception(VOID);

/* GDT table */
__attribute__((aligned(0x10))) UINT64 KiGdtEntries[] = {
    0x0000000000000000,
    0x00AF9B000000FFFF,
    0x00AF93000000FFFF
};

/* GDTR */
struct _GDT_DESCRIPTOR {
    UINT16 Limit;
    UINT64 Base;
} PACKED KiGdtDescriptor;

/* IDTR */
struct _IDT_DESCRIPTOR {
    UINT16 Limit;
    UINT64 Base;
} PACKED KiIdtDescriptor;

/* Primary IDT */
KIDT KiIdt = { 0 };

FORCE_INLINE VOID KiInstallIdtr() {
    asm volatile ("lidt %0" :: "m"(KiIdtDescriptor));
}

static VOID KiSetInterruptVector(UINT8 Index, UINT64 Handler) {
    KiIdt.Entries[Index].BaseLo = (Handler & 0xFFFF);
    KiIdt.Entries[Index].BaseMid = (Handler >> 16) & 0xFFFF;
    KiIdt.Entries[Index].BaseHi = (Handler >> 32) & 0xFFFFFFFF;
    KiIdt.Entries[Index].Selector = KERNEL_CODE_SEGMENT;
    KiIdt.Entries[Index].Flags = IDT_DESC_PRESENT | IDT_DESC_BIT32;
    KiIdt.Entries[Index].Reserved = 0;
    KiIdt.Entries[Index].Ist = 0;
}

VOID KeInitializeBsp() {
    // Prepare GDTR and install it
    KiGdtDescriptor.Base = (UINT64)&KiGdtEntries;
    KiGdtDescriptor.Limit = sizeof(KiGdtEntries);

    asm volatile (
        "lgdt %0\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        // TODO: TSS
        :: "m"(KiGdtDescriptor) : "rax"
    ); 

    // Prepare IDTR
    KiIdtDescriptor.Base = (UINT64)&KiIdt;
    KiIdtDescriptor.Limit = sizeof(KiIdt) - 1;

    KiSetInterruptVector(0, (UINT64)&KiDivisionException);
    KiSetInterruptVector(1, (UINT64)&KiDebugException);
    KiSetInterruptVector(2, (UINT64)&KiNMIException);
    KiSetInterruptVector(3, (UINT64)&KiBreakpointException);
    KiSetInterruptVector(4, (UINT64)&KiOverflowException);
    KiSetInterruptVector(5, (UINT64)&KiBoundException);
    KiSetInterruptVector(6, (UINT64)&KiInvalidOpcodeException);
    KiSetInterruptVector(7, (UINT64)&KiNoFPUException);
    KiSetInterruptVector(8, (UINT64)&KiDoubleFaultException);
    KiSetInterruptVector(9, (UINT64)&KiCoprocessorSegmentException);
    KiSetInterruptVector(10, (UINT64)&KiInvalidTSSException);
    KiSetInterruptVector(11, (UINT64)&KiSegmentNotPresentException);
    KiSetInterruptVector(12, (UINT64)&KiStackSegmentException);
    KiSetInterruptVector(13, (UINT64)&KiGeneralProtectionException);
    KiSetInterruptVector(14, (UINT64)&KiPageFaultException);
    KiSetInterruptVector(15, (UINT64)&KiReservedException);
    KiSetInterruptVector(16, (UINT64)&KiFloatingPointException);
    KiSetInterruptVector(17, (UINT64)&KiAlignmentCheck);
    KiSetInterruptVector(18, (UINT64)&KiMachineCheck);
    KiSetInterruptVector(19, (UINT64)&KiSIMDFloatingPointException);
    KiSetInterruptVector(20, (UINT64)&KiVirtualizationException);
    KiSetInterruptVector(21, (UINT64)&KiControlProtectionException);
    KiSetInterruptVector(28, (UINT64)&KiHypervisorInjectionException);
    KiSetInterruptVector(29, (UINT64)&KiVMMCommunicationException);
    KiSetInterruptVector(30, (UINT64)&KiSecurityException);
    KiSetInterruptVector(31, (UINT64)&KiReserved2Exception);

    KiInstallIdtr();
}
