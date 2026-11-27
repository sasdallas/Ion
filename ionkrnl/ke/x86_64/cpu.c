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
extern VOID KiApcSoftwareInterrupt(VOID);

extern UINT_PTR KiTrapTableStart;
extern UINT_PTR KiTrapTableEnd;

/* IDT handler table */
VOID* KiHandlerTable[256] = { 0 };

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

/* BSP KCPU */
KCPU BspCpu = { 0 };

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

static VOID KiEnableSSE() {
    UINT64 Cr0, Cr4;
    asm volatile ("mov %%cr0, %0" : "=r"(Cr0));
    Cr0 = (Cr0 & ~((1ULL << 2) | (1ULL << 3))) | (1ULL << 1);
    asm volatile ("mov %0, %%cr0" :: "r"(Cr0) : "memory");
    asm volatile ("mov %%cr4, %0" : "=r"(Cr4));
    Cr4 |= (1ULL << 9) | (1ULL << 10);
    asm volatile ("mov %0, %%cr4" :: "r"(Cr4) : "memory");
    asm volatile ("fninit");
}

VOID KiInitializeProcessor(KCPU *Cpu) {
    KiEnableSSE();

    // Prepare GDT
    asm volatile (
        "lgdt %0\n"
        "pushq $0x08\n"
        "leaq 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "movw $0x10, %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%ss\n"
        // TODO: TSS
        :: "m"(KiGdtDescriptor) : "rax"
    ); 

    // Setup CPU-specific data
    Cpu->Irql = PASSIVE_LEVEL;
    Cpu->Self = Cpu;
    UINT_PTR Address = (UINT_PTR)Cpu;
    asm volatile ("wrmsr" :: "a"((UINT32)Address), "d"((UINT32)(Address >> 32)), "c"(0xC0000102) : "memory");
    asm volatile ("wrmsr" :: "a"((UINT32)Address), "d"((UINT32)(Address >> 32)), "c"(0xC0000101) : "memory");

    KeHardwareUpdateIrql(PASSIVE_LEVEL);
    KiInstallIdtr();
}

VOID KeInitializeBsp() {
    KiGdtDescriptor.Base = (UINT64)&KiGdtEntries;
    KiGdtDescriptor.Limit = sizeof(KiGdtEntries) - 1;

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

    // APC interrupt is within exception bounds but its fine because Windows can do that,
    // and it makes IRQL determination much easier (high nibble = target IRQL)
    // (see Windows Internals if you want proof they do this shit)
    KiSetInterruptVector(APC_VECTOR, (UINT64)&KiApcSoftwareInterrupt);

    // Install common trap tables
    SIZE_T Entries = ((UINT_PTR)&KiTrapTableEnd - (UINT_PTR)&KiTrapTableStart) / sizeof(UINT_PTR);
    UINT_PTR *HandlerTable = &KiTrapTableStart;
    for (SIZE_T i = 0; i < Entries; i++) {
        KiSetInterruptVector(i + 32, HandlerTable[i]);
    }

    KiInitializeProcessor(&BspCpu);
}

VOID KiCommonTrapDispatcher(TRAP_FRAME *TrapFrame) {
    KCPU *Cpu = KeGetCpu();

    // The high nibble of the vector is the target IRQL
    UINT Vector = (UINT)TrapFrame->IntNo;
    KIRQL VectorIrql = Vector >> 4;
    KIRQL OldIrql = Cpu->Irql;

    // !!!: Hack for the spurious APIC vector
    // Raise IRQL
    if (Vector != APIC_SPURIOUS_VECTOR && VectorIrql > OldIrql) {
        KeRaiseIrql(VectorIrql, &OldIrql);
    }

    // Lookup handler.
    VOID (*Handler)(TRAP_FRAME*) = (typeof(Handler))KiHandlerTable[TrapFrame->IntNo];
    if (Handler == NULL) {
        KeBugCheckTrapFrame(TrapFrame, "Unhandled interrupt %d", TrapFrame->IntNo);
    }

    Handler(TrapFrame);

    // Finished execution, restore IRQL
    if (Vector != APIC_SPURIOUS_VECTOR && VectorIrql > OldIrql) {
        KeLowerIrql(OldIrql);
    
        // If interrupts were enabled on the trapping code,
        // we can process software interrupts here.
        if (TrapFrame->Rflags & (1 << 9)) {
            KiDispatchPendingSoftwareInterrupt(OldIrql);
        }
    }
}

IONAPI OS_STATUS KeSetInterruptHandler(UINT Vector, VOID (*Handler)(TRAP_FRAME*)) {
    if (KiHandlerTable[Vector] != NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    KiHandlerTable[Vector] = Handler;
    return STATUS_SUCCESS;
}

// NMIs are used by x86_64 to stop all other CPUs
VOID KiNmiTrapHandler(TRAP_FRAME *TrapFrame) {
    DISABLE_INTERRUPTS();
    for (;;) KeHaltExecution();
}
