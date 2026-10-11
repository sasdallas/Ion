/**
 * @file ionkrnl/ke/trap.c
 * @brief Trap handler
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ion.h>

// Trap handler for unknown/unhandled exceptions
VOID KiUnknownTrapHandler(TRAP_FRAME *TrapFrame) {
    KeBugCheckTrapFrame(TrapFrame, "Unknown fatal trap %d detected", TRAP_VECTOR(TrapFrame));
}

VOID KiPageFaultHandler(TRAP_FRAME *TrapFrame) {
    KeBugCheckTrapFrame(TrapFrame, "Unhandled page fault detected at %p", TRAP_ADDR(TrapFrame));
}
