/**
 * @file ionkrnl/ex/init.c
 * @brief Executive initialization code
 * 
 * Finishes the upper initialization stages of the kernel, post-scheduler initialization
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ex.h>
#include <ion/ke/ps.h>
#include <ion/ke/ob.h>
#include <ion/ke/io.h>
#include <ion/ke/ke.h>
#include <ion/ke/ldr.h>
#include <ion/ke/io.h>
#include <ion/ke/os.h>
#include <ion/ion.h>

#define INIT_OR_BUGCHECK(fn) ({ OS_STATUS Status = fn(); if (!OS_SUCCESS(Status)) { KeBugCheckWithStatus(Status, #fn " failed to initialize (Ex stage)"); }});

static OS_STATUS ExpInitializeTypes() {
    OBJECT_TYPE_ATTRIBUTES Attributes = {
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = NULL,
        .DumpProcedure = NULL,
        .InvalidAttributes = 0,
        .TargetHeap = MmPagedHeap
    };

    OS_STATUS Status = ObCreateObjectType("Event", &Attributes, &ExEventObjectType);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}

// Load fireworks demo if present
static VOID ExpStartFireworks(VOID) {
    LOADER_PARAMETER_BLOCK *Loader = KeGetLoaderParameterBlock();
    for (SIZE_T i = 1; i < Loader->NrLoadedModules; i++) {
        LOADER_MODULE *Module = &Loader->LoadedModules[i];

        if (Module->Cmdline == NULL || !RtlStringEquals(Module->Cmdline, "/fwt")) {
            continue;
        }

        LOADED_DLL *Dll;
        OS_STATUS Status = LdrLoadDll(Module->Base, Module->Size, &Dll);

        if (!OS_SUCCESS(Status)) {
            DbgPrint("Fwt.dll failed to load DLL 0x%08x\n", Status);
            return;
        }
        OS_STATUS (*FwtStart)(VOID) = (typeof(FwtStart))LdrGetSymbol(Dll, "FwtStart");
        if (!FwtStart) {
            DbgPrint("FwtStart not found!!!\n");
            return;
        }

        FwtStart();
        return;
    }
}

NO_RETURN VOID ExInitialize() {
    DbgPrint("ExInitialize()\n");

    // Object manager
    INIT_OR_BUGCHECK(ObInitialize);

    // Prepare types (drivers might use them)
    INIT_OR_BUGCHECK(ExpInitializeTypes);

    // IO
    INIT_OR_BUGCHECK(IoInitialize);

    // Fireworks
    ExpStartFireworks();

extern VOID  ObpDumpObjectTree();
    ObpDumpObjectTree();

    for (;;) {
        PsYieldThread();
    }
}
