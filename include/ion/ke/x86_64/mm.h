/**
 * @file include/ion/ke/x86_64/mm.h
 * @brief Mm definitions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_X86_64_MM_H
#define _ION_KE_X86_64_MM_H 1

#include <ion/base.h>
#include <ion/ke/mm/addrspace.h>

#define PAGE_SIZE 0x1000
#define PAGE_SHIFT 12

// Memory mapping on x86_64
#define MM_KERNEL_START ((UINT_PTR)0xFFFF800000000000ULL)
#define MM_KERNEL_END   ((UINT_PTR)0xFFFFFFFFFFFFF000ULL)
#define MM_PFNDB_ADDR   ((UINT_PTR)0xFFFFFA8000000000ULL)

// PTE flags
#define MM_PTE_PRESENT      (1ULL << 0)
#define MM_PTE_WRITABLE     (1ULL << 1)
#define MM_PTE_USER         (1ULL << 2)
#define MM_PTE_PWT          (1ULL << 3)
#define MM_PTE_PCD          (1ULL << 4)
#define MM_PTE_PS           (1ULL << 7)
#define MM_PTE_PAT          (1ULL << 7) // leaf 4 KiB PTE only
#define MM_PTE_NX           (1ULL << 63)
#define MM_PTE_ADDR_MASK    0x000FFFFFFFFFF000ULL

typedef UINT64 MMPTE;

FORCE_INLINE MM_PAGETBL *MmuCurrentPageTable() {
    UINT_PTR PageTable;
    asm volatile ("movq %%cr3, %0" : "=r"(PageTable));
    return (MM_PAGETBL*)PageTable;
}

#endif
