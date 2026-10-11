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

typedef UCHAR KIRQL;

#define PASSIVE_LEVEL           0x00
#define APC_LEVEL               0x01
#define DISPATCH_LEVEL          0x02

#define DISABLE_INTERRUPTS() asm volatile ("cli")
#define ENABLE_INTERRUPTS() asm volatile ("sti")

FORCE_INLINE VOID KeHaltExecution() {
    asm volatile ("hlt");
}

FORCE_INLINE VOID KeStallExecution() {
    asm volatile ("pause");
}

#endif
