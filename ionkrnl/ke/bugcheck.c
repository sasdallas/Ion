/**
 * @file ionkrnl/ke/bugcheck.c
 * @brief KeBugCheck family
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

IONAPI NO_RETURN VOID KeBugCheckWithMessage(const CHAR* Message, ...) {
    // TODO: Improve the formatting on this
    va_list ap;
    va_start(ap, Message);
    char Buffer[512];
    RtlFormatStringVa(Buffer, 512, Message, ap);
    va_end(ap);

    DbgPrintMsg("\nFatal exception trap. The system has been halted.\n");
    DbgPrintMsg("\033[31mSTOP: \033[0m%s\n", Buffer);
    DbgPrintMsg("\nPlease report this issue.\n");

    // Halt forever (todo move)
    DISABLE_INTERRUPTS();
    for (;;) {
        KeHaltExecution();
    }
}

IONAPI NO_RETURN VOID KeBugCheckTrapFrame(TRAP_FRAME *TrapFrame, const CHAR* Message, ...) {
    // TODO: Improve the formatting on this
    va_list ap;
    va_start(ap, Message);
    char Buffer[512];
    RtlFormatStringVa(Buffer, 512, Message, ap);
    va_end(ap);

    DbgPrintMsg("\nFatal exception trap. The system has been halted.\n");
    DbgPrintMsg("\033[31mSTOP: \033[0m%s\n", Buffer);
    
    DbgPrintMsg("\n");
    KePrintTrapFrame(TrapFrame);
    DbgPrintMsg("\nPlease report this issue.\n");

    // Halt forever (todo move)
    DISABLE_INTERRUPTS();
    for (;;) {
        KeHaltExecution();
    }
}
