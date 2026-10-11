/**
 * @file include/ion/ke/mm/pfn.h
 * @brief Mm PFN implementation
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_MM_PFN_H
#define _ION_KE_MM_PFN_H 1

#include <ion/base.h>

typedef ULONG_PTR PFN_NUMBER;

#define PFN_INVALID ((PFN_NUMBER)(-1))

#define MM_PFN_FREE         0
#define MM_PFN_ALLOCATED    1

typedef struct _MMPFN {
    struct {
        UINT Type:4;
    };

    INT RefCount;

    /* Freelist pointers for PMM */ 
    struct {
        PFN_NUMBER Next;
        PFN_NUMBER Prev;
    } Alloc;
} MMPFN;

extern MMPFN *MmPfnDatabase;

#endif

