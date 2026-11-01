/**
 * @file ionkrnl/ke/x86_64/trap.S
 * @brief Trap code for Ion
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

.code64

.extern KiUnknownTrapHandler
.extern KiPageFaultHandler
.extern KiCommonTrapDispatcher

/* Swap GS base */
.macro _swapgs
    // We only need to swapgs on a usermode -> kernel mode interrupt
    testq $3, 24(%rsp) // 24(%rsp) = CS pushed by CPU
    jz 1f
    swapgs
1:
.endm


.macro PUSH_REGISTERS
    _swapgs // If usermode, then swapgs to switch to kernel GS base

    // AMD killed PUSHA for some weird reason, so we'll have to
    // do this manually - and this time we go alphabetical.
    pushq %rax
    pushq %rbx 
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp

    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8

    movq %cr2, %rax
    pushq %rax
.endm

/* Restore registers macro */
.macro RESTORE_REGISTERS
    addq $8, %rsp
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15

    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx 
    popq %rax

    cli
    _swapgs
    addq $16, %rsp
.endm

.macro CALL_TRAP_HANDLER dest
    cld
    movq %rsp, %rcx
    andq $-16, %rsp
    subq $576, %rsp              // Shadow space, trap-frame pointer, and aligned FPU state.
    movq %rcx, 32(%rsp)
    fxsave64 48(%rsp)
    call \dest
    fxrstor64 48(%rsp)
    movq 32(%rsp), %rsp
.endm

.macro TRAP_HANDLER_ERR name index dest
    .global \name
    \name:
        // The CPU has already pushed the error code.
        pushq $\index
        PUSH_REGISTERS
        CALL_TRAP_HANDLER \dest
        RESTORE_REGISTERS
        iretq
.endm

.macro TRAP_HANDLER_NOERR name index dest
    .global \name
    \name:
        pushq $0
        pushq $\index
        PUSH_REGISTERS
        CALL_TRAP_HANDLER \dest
        RESTORE_REGISTERS
        iretq
.endm

.macro TRAP_HANDLER index
    TRAP_HANDLER_NOERR KiTrapHandler\index, \index + 32, KiCommonTrapDispatcher
.endm

.macro TRAP_TABLE_ENTRY index
    .quad KiTrapHandler\index
.endm

/* Traps */
TRAP_HANDLER_NOERR   KiDivisionException,               0,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiDebugException,                  1,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiNMIException,                    2,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiBreakpointException,             3,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiOverflowException,               4,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiBoundException,                  5,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiInvalidOpcodeException,          6,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiNoFPUException,                  7,  KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiDoubleFaultException,            8,  KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiCoprocessorSegmentException,     9,  KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiInvalidTSSException,             10, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiSegmentNotPresentException,      11, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiStackSegmentException,           12, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiGeneralProtectionException,      13, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiPageFaultException,              14, KiPageFaultHandler
TRAP_HANDLER_NOERR   KiReservedException,               15, KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiFloatingPointException,          16, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiAlignmentCheck,                  17, KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiMachineCheck,                    18, KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiSIMDFloatingPointException,      19, KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiVirtualizationException,         20, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiControlProtectionException,      21, KiUnknownTrapHandler
/* 22 - 27 are reserved */
TRAP_HANDLER_NOERR   KiHypervisorInjectionException,    28, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiVMMCommunicationException,       29, KiUnknownTrapHandler
TRAP_HANDLER_ERR     KiSecurityException,               30, KiUnknownTrapHandler
TRAP_HANDLER_NOERR   KiReserved2Exception,              31, KiUnknownTrapHandler

.altmacro
.set i, 0
.rept 224 // 256 - 32
    TRAP_HANDLER %i
    .set i, i+1
.endr

.global KiTrapTableStart
KiTrapTableStart:
.set i, 0
.rept 224
    TRAP_TABLE_ENTRY %i
    .set i, i+1
.endr

.global KiTrapTableEnd
KiTrapTableEnd:
