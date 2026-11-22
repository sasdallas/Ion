/**
 * @file hal/halx86/hal/clock.c
 * @brief x86 clock interface
 * 
 * Supports invariant TSC only for now, and this is pretty crap.
 * Just here so I can get Halx86QueryMonotonicTime working
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include "hal.h"
#include <ion/ion.h>
#include <ion/ke/ke.h>
#include <ion/x86_64/port.h>

#define PIT_FREQUENCY 1193182ULL
#define PIT_CALIBRATION_COUNT 59659U /* about 50 ms */

/* TSC information */
UINT64 HalpBaseTsc = 0;
UINT64 HalpTscFrequency = 0;
UINT64 HalpTscMultiplier = 0;

FORCE_INLINE UINT64 __rdtsc() {
    UINT32 Low;
    UINT32 High;
    asm volatile ("rdtsc" : "=a"(Low), "=d"(High) :: "memory");
    return ((UINT64)High << 32) | Low;
}

UINT64 Halx86QueryMonotonicTime() {
    return (UINT64)(((__uint128_t)(__rdtsc() - HalpBaseTsc) * HalpTscMultiplier) >> 32);
}

OS_STATUS HalClockInitialize() {
    UINT8 Speaker = PortRead8(0x61);
    PortWrite8(0x61, Speaker & ~0x03);
    PortWrite8(0x42, PIT_CALIBRATION_COUNT & 0xFF);
    PortWrite8(0x42, PIT_CALIBRATION_COUNT >> 8);

    UINT64 Start = __rdtsc();
    PortWrite8(0x61, (Speaker & ~0x02) | 0x01);

    UINT32 Timeout = 100000000;
    while (!(PortRead8(0x61) & 0x20) && --Timeout) {
        KeStallExecution();
    }

    UINT64 End = __rdtsc();
    PortWrite8(0x61, Speaker);

    if (Timeout == 0) {
        DbgPrint("What lol\n");
        return STATUS_INTERNAL_ERROR;
    }

    HalpTscFrequency = ((End - Start) * PIT_FREQUENCY + PIT_CALIBRATION_COUNT / 2) / PIT_CALIBRATION_COUNT;
    ASSERT(HalpTscFrequency > 0);

    HalpTscMultiplier = (UINT64)((__uint128_t)1000000000ULL << 32) / HalpTscFrequency;
    HalpBaseTsc = __rdtsc();

    return STATUS_SUCCESS;
}
