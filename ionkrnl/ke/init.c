/**
 * @file ionkrnl/ke/init.c
 * @brief KeInitialize code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>

/* Loader block */
LOADER_PARAMETER_BLOCK *KeLoaderParameterBlock = NULL;

#define INIT_OR_BUGCHECK(fn) ({ OS_STATUS Status = fn(); if (!OS_SUCCESS(Status)) { KeBugCheckWithMessage("Failed to initialize " #fn ": 0x%08x", Status); }});

NO_RETURN VOID KeInitialize() {
    /* BSP init prepares the traps */
    KeInitializeBsp();

    /* Start with Mm init */
    INIT_OR_BUGCHECK(MiInitializePfnDatabase);
    INIT_OR_BUGCHECK(MiInitializeKernelArena);
    INIT_OR_BUGCHECK(MiUnmarkUsedMemory);
    INIT_OR_BUGCHECK(MiInitializePoolManager);
    INIT_OR_BUGCHECK(MiInitializeKernelHeaps);

    DbgPrint("KeInitialize has finished, halting!\n");

    DISABLE_INTERRUPTS();
    for (;;) {
        KeHaltExecution();
    }
}

