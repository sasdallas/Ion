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
#include <ion/ke/ldr.h>

#ifdef __x86_64__

// todo: move
VOID KeGetBacktrace(VOID *Frame, UINT_PTR Address) {
    UINT_PTR *Current = Frame;
    UINT_PTR Start = (UINT_PTR)Frame;
    UINT_PTR Limit = Start + 0x10000;
    DbgPrintMsg("\nBacktrace:\n");

    for (UINT32 i = 0; i < 64; i++) {
        const CHAR *DllName, *SymbolName;
        UINT_PTR Offset;
        if (LdrFindSymbolByAddress(Address, &DllName, &SymbolName, &Offset)) {
            if (SymbolName != NULL) {
                DbgPrintMsg("  %p %s!%s+0x%lx\n", (VOID*)Address, DllName, SymbolName, Offset);
            } else {
                DbgPrintMsg("  %p %s+0x%lx\n", (VOID*)Address, DllName, Offset);
            }
        } else {
            DbgPrintMsg("  %p\n", (VOID*)Address);
        }

        if ((UINT_PTR)Current < Start || (UINT_PTR)Current > Limit - 2 * sizeof(UINT_PTR) || ((UINT_PTR)Current & (sizeof(UINT_PTR) - 1))) {
            break;
        }

        // RBP can point inside a frame too
        BOOLEAN Found = FALSE;
        for (SIZE_T Slot = 0; Slot < 5; Slot++) {
            UINT_PTR *Saved = Current + Slot;
            if ((UINT_PTR)Saved > Limit - 2 * sizeof(UINT_PTR)) break;

            UINT_PTR Next = Saved[0];
            UINT_PTR Return = Saved[1];
            if (Next <= (UINT_PTR)Saved || Next > Limit || Return == 0) continue;

            const CHAR *CallerDll;
            const CHAR *CallerSymbol;
            UINT_PTR CallerOffset;
            if (!LdrFindSymbolByAddress(Return - 1, &CallerDll, &CallerSymbol, &CallerOffset)) {
                continue;
            }

            Address = Return - 1;
            Current = (UINT_PTR*)Next;
            Found = TRUE;
            break;
        }
        if (!Found) break;
    }
}

#else

VOID KeGetBacktrace(VOID*, UINT_PTR) {
    DbgPrintMsg("\nKeGetBacktrace is not implemented on this architecture.\n");
}

#endif

IONAPI NO_RETURN VOID KeBugCheckWithMessage(const CHAR* Message, ...) {
    // TODO: Improve the formatting on this
    va_list ap;
    va_start(ap, Message);
    char Buffer[512];
    RtlFormatStringVa(Buffer, 512, Message, ap);
    va_end(ap);

    DbgPrintMsg("\nFatal exception trap. The system has been halted.\n");
    DbgPrintMsg("\033[31mSTOP: \033[0m%s\n", Buffer);


    KeGetBacktrace((VOID*)(*(UINT_PTR*)__builtin_frame_address(0)), (UINT_PTR)__builtin_return_address(0) - 1);

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
    KeGetBacktrace((VOID*)TRAP_FRAME(TrapFrame), TRAP_PC(TrapFrame));
    DbgPrintMsg("\nPlease report this issue.\n");

    // Halt forever (todo move)
    DISABLE_INTERRUPTS();
    for (;;) {
        KeHaltExecution();
    }
}

IONAPI NO_RETURN VOID KeBugCheckWithStatus(OS_STATUS Status, const CHAR* Message, ...) {
    // TODO: Improve the formatting on this
    va_list ap;
    va_start(ap, Message);
    char Buffer[512];
    RtlFormatStringVa(Buffer, 512, Message, ap);
    va_end(ap);

    DbgPrintMsg("\nFatal exception trap. The system has been halted.\n");
    DbgPrintMsg("\033[31mSTOP: \033[0m%s\n", Buffer);
    DbgPrintMsg("      %s (0x%08x)\n", RtlStatusToString(Status), Status);
    KeGetBacktrace((VOID*)(*(UINT_PTR*)__builtin_frame_address(0)), (UINT_PTR)__builtin_return_address(0) - 1);
    DbgPrintMsg("\nPlease report this issue.\n");

    // Halt forever (todo move)
    DISABLE_INTERRUPTS();
    for (;;) {
        KeHaltExecution();
    }
}
