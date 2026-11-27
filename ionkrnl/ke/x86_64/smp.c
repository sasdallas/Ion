/**
 * @file ionkrnl/ke/x86_64/smp.c
 * @brief Architecture-specific SMP code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#if _SMP

#include <ion/ke/ke.h>
#include <ion/ke/lpb.h>
#include <ion/ke/ps.h>
#include <ion/hal.h>
#include <ion/third_party/limine.h>

#define AP_TIMEOUT 1000000000ULL // 1s for AP to boot

// This is a shared structure between the CPUs used to synchronize the BSP's
// main initialization + the AP. Has to be allocated from memory map memory!!
typedef struct _AP_BOOTSTRAP {
    KCPU Cpu;
    BOOLEAN ApStarted;
} AP_BOOTSTRAP;

/* Bootstrap code */
static AP_BOOTSTRAP *KiApBootstrap = NULL;

/* Mp request */
extern volatile struct LIMINE_MP(request) KiLimineMpRequest;
extern VOID* KeAllocateFromMemmap(SIZE_T Size);

/**
 * AP entry
 */
static NO_RETURN __attribute__((sysv_abi)) VOID KiApEntry(struct LIMINE_MP(info) *Info) {
    DISABLE_INTERRUPTS();

    // Prepare KCPU
    AP_BOOTSTRAP *Bootstrap = (AP_BOOTSTRAP*)(UINT_PTR)Info->extra_argument;
    KiInitializeProcessor(&Bootstrap->Cpu);

    // AP initialize
    KeInitializeAp();

    // AP is done starting up
    __atomic_store_n(&Bootstrap->ApStarted, TRUE, __ATOMIC_RELEASE);

    PsSwitchToNextThread();
    for (;;) KeHaltExecution();
}

/**
 * Called by KeInitialize to start the other APs
 */
VOID KiStartApplicationProcessors() {
    struct LIMINE_MP(response) *Mp = KiLimineMpRequest.response;
    if (Mp == NULL) {
        DbgPrint("KiStartApplicationProcessors(): No Limine response\n");
        return;
    }

    // Limine generously gives us our lapic ID
    KeGetCpu()->HardwareId = Mp->bsp_lapic_id;

    // Start up each AP
    UINT32 LogicalId = 1;
    for (SIZE_T i = 0; i < Mp->cpu_count; i++) {
        struct LIMINE_MP(info) *Info = Mp->cpus[i];
        if (Info->lapic_id == Mp->bsp_lapic_id) continue;
        
        AP_BOOTSTRAP *Bootstrap = &KiApBootstrap[i];
        RtlZeroMemory(Bootstrap, sizeof(AP_BOOTSTRAP));
        Bootstrap->Cpu.LogicalId = LogicalId++;
        Bootstrap->Cpu.HardwareId = Info->lapic_id;

        Info->extra_argument = (UINT_PTR)Bootstrap;

        __atomic_store_n(&Info->goto_address, (limine_goto_address)KiApEntry, __ATOMIC_RELEASE);
    
        // Timeout
        UINT64 Start = HalQueryMonotonicTime();
        while (!__atomic_load_n(&Bootstrap->ApStarted, __ATOMIC_ACQUIRE)) {
            if (HalQueryMonotonicTime() - Start >= AP_TIMEOUT) {
                break;
            }

            KeStallExecution();
        }

        if (__atomic_load_n(&Bootstrap->ApStarted, __ATOMIC_ACQUIRE) == FALSE) {
            DbgPrint("CPU%u (APIC ID %u) did not reach AP initialization!\n", Bootstrap->Cpu.LogicalId, Info->lapic_id);
        }
    }
}

VOID KiAllocateBootstrapInformation() {
    struct LIMINE_MP(response) *Mp = KiLimineMpRequest.response;
    if (Mp == NULL || Mp->cpu_count == 1) {
        return;
    }

    KiApBootstrap = KeAllocateFromMemmap(Mp->cpu_count * sizeof(AP_BOOTSTRAP));
}

#endif
