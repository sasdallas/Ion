/**
 * @file include/ion/ke/mm.h
 * @brief Mm
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_MM_H
#define _ION_KE_MM_H 1

#include <ion/base.h>

#ifdef __x86_64__
#include <ion/ke/x86_64/mm.h>
#else
#error unknown architecture
#endif

#include <ion/ke/mm/pfn.h>

FORCE_INLINE PFN_NUMBER MmConvertPhysToPfn(UINT_PTR Physical) {
    return (PFN_NUMBER)(Physical >> PAGE_SHIFT);
}

FORCE_INLINE PFN_NUMBER MmGetPfnNumber(MMPFN *Pfn) {
    return (PFN_NUMBER)(((UINT_PTR)Pfn - (UINT_PTR)MmPfnDatabase) / sizeof(MMPFN));
}

/* Mi: Memory initialization */
OS_STATUS MiInitializePfnDatabase();
UINT_PTR MiAllocatePageFromMemoryMap();
UINT_PTR MiConvertToHhdm(UINT_PTR Address);

/* Mm: Memory management */

// Allocate a physical page
PFN_NUMBER MmAllocatePhysicalPage();


#endif
