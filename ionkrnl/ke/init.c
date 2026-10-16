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
#include <ion/ke/ldr.h>
#include <ion/hal.h>
#include <ion/ion.h>

/* Loader block */
LOADER_PARAMETER_BLOCK *KeLoaderParameterBlock = NULL;

#define INIT_OR_BUGCHECK(fn) ({ OS_STATUS Status = fn(); if (!OS_SUCCESS(Status)) { KeBugCheckWithStatus(Status, #fn " failed to initialize"); }});

IONAPI LOADER_PARAMETER_BLOCK *KeGetLoaderParameterBlock() {
    return KeLoaderParameterBlock;
}

OS_STATUS KiInitializeHal() {
    OS_STATUS Status = LdrInitializeHal();
    if (!OS_SUCCESS(Status)) return Status;

    Status = HalInitSystemConsole();
    if (!OS_SUCCESS(Status)) {
        DbgPrint("HalInitSystemConsole failed: %s (0x%08x)\n", RtlStatusToString(Status), Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

NO_RETURN VOID KeInitialize() {
    // Traps
    KeInitializeBsp();

    // Mm init
    INIT_OR_BUGCHECK(MiInitializePfnDatabase);
    INIT_OR_BUGCHECK(MiInitializeKernelArena);
    INIT_OR_BUGCHECK(MiUnmarkUsedMemory);
    INIT_OR_BUGCHECK(MiMapFramebuffers);
    INIT_OR_BUGCHECK(MiInitializePoolManager);
    INIT_OR_BUGCHECK(MiInitializeKernelHeaps);

    // Ldr
    INIT_OR_BUGCHECK(LdrInitialize);

    // HAL (+ system console)
    INIT_OR_BUGCHECK(KiInitializeHal);
    HalConsolePrintf("Ion Operating System\n");
    HalConsolePrintf("Copyright (C) Ion Development Team 2026\n");
    HalConsolePrintf("Total physical memory: %zu kB (%zu kB in use)\n\n", MmTotalPhysicalMemory / 1024, MmUsedPhysicalMemory / 1024);

    DbgPrint("KeInitialize has finished, halting!\n");

    DISABLE_INTERRUPTS();
    for (;;) {
        KeHaltExecution();
    }
}

