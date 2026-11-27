/**
 * @file include/ion/ke/x86_64/ke_types.h
 * @brief Ke types for x86_64
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_X86_64_KE_H
#define _ION_KE_X86_64_KE_H 1

#include <ion/base.h>
#include <ion/rtl/dbg.h>
#include <ion/ke/ke_types.h>

typedef struct _TRAP_FRAME {
    UINT64 Cr2;
    UINT64 R8, R9, R10, R11, R12, R13, R14, R15;
    UINT64 Rbp, Rdi, Rsi, Rdx, Rcx, Rbx, Rax;
    UINT64 IntNo, ErrCode;
    UINT64 Rip, Cs, Rflags, Rsp, Ss;
} TRAP_FRAME;

#define KERNEL_CODE_SEGMENT     0x08
#define NMI_VECTOR              0x02

#define PASSIVE_LEVEL           0x00
#define APC_LEVEL               0x01
#define DISPATCH_LEVEL          0x02
#define DEVICE_LEVEL            0x03
#define CLOCK_LEVEL             0x04

/**
 * Each vector here has the high nibble of the byte reserved for the
 * APIC priority class.
 */
#define DISPATCH_VECTOR         0x2F
#define APC_VECTOR              0x1F
#define CLOCK_VECTOR            0x40
#define APIC_SPURIOUS_VECTOR    0xDF

#define DISABLE_INTERRUPTS() asm volatile ("cli")
#define ENABLE_INTERRUPTS() asm volatile ("sti")

VOID KiInitializeProcessor(KCPU *Cpu);
VOID KiStartApplicationProcessors(VOID);

#define TRAP_PC(TrapFrame) ((TrapFrame)->Rip)
#define TRAP_ADDR(TrapFrame) ((TrapFrame)->Cr2)
#define TRAP_CODE(TrapFrame) ((TrapFrame)->ErrCode)
#define TRAP_VECTOR(TrapFrame) ((TrapFrame)->IntNo)
#define TRAP_FRAME(TrapFrame) ((TrapFrame)->Rbp)

FORCE_INLINE VOID KeHaltExecution() {
    asm volatile ("hlt");
}

FORCE_INLINE VOID KeStallExecution() {
    asm volatile ("pause");
}

FORCE_INLINE VOID KePrintTrapFrame(TRAP_FRAME *TrapFrame) {
    DbgPrintMsg("RAX %016llX RBX %016llX RCX %016llX RDX %016llX\n", TrapFrame->Rax, TrapFrame->Rbx, TrapFrame->Rcx, TrapFrame->Rdx);
    DbgPrintMsg("RDI %016llX RSI %016llX RBP %016llX RSP %016llX\n", TrapFrame->Rdi, TrapFrame->Rsi, TrapFrame->Rbp, TrapFrame->Rsp);
    DbgPrintMsg("R8  %016llX R9  %016llX R10 %016llX R11 %016llX\n", TrapFrame->R8, TrapFrame->R9, TrapFrame->R10, TrapFrame->R11);
    DbgPrintMsg("R12 %016llX R13 %016llX R14 %016llX R15 %016llX\n", TrapFrame->R12, TrapFrame->R13, TrapFrame->R14, TrapFrame->R15);
    DbgPrintMsg("ERR %016llX RFL %016llX\n", TrapFrame->ErrCode, TrapFrame->Rflags);
    DbgPrintMsg("PC  %04x:%016llX\n", TrapFrame->Cs, TrapFrame->Rip);
}

FORCE_INLINE KCPU *KeGetCpu() {
    KCPU *CpuObject;
    asm volatile ("mov %%gs:%c1, %0" : "=r" (CpuObject) : "i"(FIELD_OFFSET(KCPU, Self)));
    return CpuObject;
}

FORCE_INLINE THREAD *KeGetCurrentThread() {
    THREAD *Thread;
    asm volatile ("mov %%gs:%c1, %0" : "=r" (Thread) : "i"(FIELD_OFFSET(KCPU, CurrentThread)));
    return Thread;
}

FORCE_INLINE BOOLEAN KeSetInterruptState(BOOLEAN InterruptState) {
    UINT64 Rflags;
    asm volatile (  "pushf\n"
                    "pop %0" : "=rm"(Rflags) :: "memory"
    );

    if (InterruptState) {
        asm volatile ("sti" ::: "memory");
    } else {
        asm volatile ("cli" ::: "memory");
    }

    return !!(Rflags & (1 << 9));
}

FORCE_INLINE VOID KeHardwareUpdateIrql(KIRQL NewIrql) {
    asm volatile ("mov %0, %%cr8" :: "r"((UINT64)NewIrql) : "memory");
}

FORCE_INLINE unsigned long long __readmsr(const int Reg) {
    UINT32 Low, High;
    asm volatile ("rdmsr" : "=a"(Low), "=d"(High) : "c"(Reg));
    return ((UINT64)High << 32) | Low;
}

#endif

