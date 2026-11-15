/**
 * @file hal/halx86/hal/hal.h
 * @brief HALx86
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _HALX86_H
#define _HALX86_H 1

#include <ion/base.h>

/* ACPI */
OS_STATUS HalAcpiInitialize();

/* APIC */
OS_STATUS HalApicInitialize();

/* Clock */
OS_STATUS HalClockInitialize();

#endif
