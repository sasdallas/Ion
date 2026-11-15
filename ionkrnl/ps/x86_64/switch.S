/**
 * @file ionkrnl/ps/x86_64/switch.S
 * @brief Kernel thread context switch
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

.code64

// RCX = old context, RDX = new context, R8 = outgoing thread
.global PspSwitchContext
PspSwitchContext:
    movq %r15, 0(%rcx)
    movq %r14, 8(%rcx)
    movq %r13, 16(%rcx)
    movq %r12, 24(%rcx)
    movq %rdi, 32(%rcx)
    movq %rsi, 40(%rcx)
    movq %rbp, 48(%rcx)
    movq %rbx, 56(%rcx)
    leaq 8(%rsp), %rax
    movq %rax, 64(%rcx)
    movq (%rsp), %rax
    movq %rax, 72(%rcx)
    leaq 95(%rcx), %rax

    andq $-16, %rax
    fxsave64 (%rax)

    leaq 95(%rdx), %rax

    andq $-16, %rax
    fxrstor64 (%rax)

    movq 0(%rdx), %r15
    movq 8(%rdx), %r14
    movq 16(%rdx), %r13
    movq 24(%rdx), %r12
    movq 32(%rdx), %rdi
    movq 40(%rdx), %rsi
    movq 48(%rdx), %rbp
    movq 56(%rdx), %rbx
    movq 64(%rdx), %rsp

    movq %r8, %rax

    jmp *72(%rdx)

.global PspThreadEntry
PspThreadEntry:
    movq %rax, %rcx
    movq %r12, %rdx
    movq %r13, %r8
    subq $40, %rsp
    call PsThreadStartup
    ud2

