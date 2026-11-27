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
#include <ion/ke/ob.h>
#include <ion/ke/ldr.h>
#include <ion/ke/ps.h>
#include <ion/ke/ex.h>
#include <ion/hal.h>
#include <ion/ion.h>

/* Loader block */
LOADER_PARAMETER_BLOCK *KeLoaderParameterBlock = NULL;

#define INIT_OR_BUGCHECK(fn) ({ OS_STATUS Status = fn(); if (!OS_SUCCESS(Status)) { KeBugCheckWithStatus(Status, #fn " failed to initialize"); }});

VOID KeTestThread(VOID *Context) {
    for (;;) {
        char str[] = { KeGetCurrentThread()->ThreadId + 0x60 - 1, 0};
        HalPrintString(str);
        PsYieldThread();
    }
}

VOID KeInitializeAp() {
    // Ke per-CPU
    KiInitializeDpcQueue();
    KeInitializeTimerSystem();

    // Ps per-CPU
    INIT_OR_BUGCHECK(PsInitializeScheduler);
    INIT_OR_BUGCHECK(PsInitializeIdleThread);

    // HAL
    INIT_OR_BUGCHECK(HalInitializeAp);
}

OS_STATUS KiInitializeHal() {
    OS_STATUS Status = LdrInitializeHal();
    if (!OS_SUCCESS(Status)) return Status;

    HalConsolePrintf("Ion Operating System\n");
    HalConsolePrintf("Copyright (C) Ion Development Team 2026\n");
    HalConsolePrintf("Total physical memory: %zu kB (%zu kB in use)\n\n", MmTotalPhysicalMemory / 1024, MmUsedPhysicalMemory / 1024);

    Status = HalInitializeHardware();
    if (!OS_SUCCESS(Status)) return Status;

    HalConsolePrintf("Monotonic time: %llu ns\n", HalQueryMonotonicTime());

    return STATUS_SUCCESS;
}

NO_RETURN VOID KeInitialize() {
    // Traps
    KeInitializeBsp();

    // Other Ke stuff
    KiInitializeDpcQueue();
    KeInitializeTimerSystem();

    // Mm init
    INIT_OR_BUGCHECK(MiInitializePfnDatabase);
    INIT_OR_BUGCHECK(MiInitializeKernelAddressSpace);
    INIT_OR_BUGCHECK(MiInitializeKernelArena);
    INIT_OR_BUGCHECK(MiUnmarkUsedMemory);
    INIT_OR_BUGCHECK(MiMapFramebuffers);
    INIT_OR_BUGCHECK(MiInitializePoolManager);
    INIT_OR_BUGCHECK(MiInitializeKernelHeaps);

    // Ldr
    INIT_OR_BUGCHECK(LdrInitialize);

    // HAL
    INIT_OR_BUGCHECK(KiInitializeHal);

    // Ob
    INIT_OR_BUGCHECK(ObInitializeTypeManager);

    // Ps
    INIT_OR_BUGCHECK(PsInitialize);
    INIT_OR_BUGCHECK(PsInitializeSystemProcess);
    INIT_OR_BUGCHECK(PsInitializeIdleProcess);
    INIT_OR_BUGCHECK(PsInitializeIdleThread);
    INIT_OR_BUGCHECK(PsInitializeScheduler);

    // SMP time
#if _SMP
    KiStartApplicationProcessors();
#endif

    // Ready
    ENABLE_INTERRUPTS();

    // Ps initialization already setup our init thread
    ExInitialize();
    for (;;);
}

IONAPI LOADER_PARAMETER_BLOCK *KeGetLoaderParameterBlock() {
    return KeLoaderParameterBlock;
}
