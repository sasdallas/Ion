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

// Mm includes
#include <ion/ke/mm/pfn.h>
#include <ion/ke/mm/arena.h>

// Flags
#define MM_ALLOC_FIXED          0x01    // The allocation being requested is fixed at AddressHint, and should fail otherwise
#define MM_ALLOC_FIXED_REPLACE  0x02    // The allocation being requested is fixed at AddressHint, but should replace an existing mapping
#define MM_ALLOC_UNBACKED       0x04    // Do not back this virtual memory

// Protection
#define MM_PROT_READ            0x01
#define MM_PROT_WRITE           0x02
#define MM_PROT_EXEC            0x04

FORCE_INLINE PFN_NUMBER MmConvertPhysToPfn(UINT_PTR Physical) {
    return (PFN_NUMBER)(Physical >> PAGE_SHIFT);
}

FORCE_INLINE PFN_NUMBER MmGetPfnNumber(MMPFN *Pfn) {
    return (PFN_NUMBER)(((UINT_PTR)Pfn - (UINT_PTR)MmPfnDatabase) / sizeof(MMPFN));
}

FORCE_INLINE UINT_PTR MmConvertPfnToPhys(PFN_NUMBER PfnNumber) {
    return (UINT_PTR)(PfnNumber << PAGE_SHIFT);
}

/* Mi: Memory initialization */
OS_STATUS MiInitializePfnDatabase();
OS_STATUS MiInitializeKernelArena();
OS_STATUS MiUnmarkUsedMemory();
UINT_PTR MiAllocatePageFromMemoryMap();
UINT_PTR MiConvertToHhdm(UINT_PTR Address);

/* Mmu: Memory manager unit */
VOID MmuMapPage(UINT_PTR PageTable, UINT_PTR VirtualAddress, UINT_PTR PhysicalAddress, UINT Protection);
VOID MmuUnmapPage(UINT_PTR PageTable, UINT_PTR VirtualAddress);
VOID MmuInvalidateRange(UINT_PTR Start, SIZE_T NumPages);
UINT_PTR MmuGetPhysicalAddress(UINT_PTR PageTable, UINT_PTR VirtualAddress);

/* Mm: Memory management */
PFN_NUMBER MmAllocatePhysicalPage();
VOID MmFreePhysicalPage(PFN_NUMBER PfnNumber);
VOID MmHoldPhysicalPage(MMPFN *Pfn);
VOID MmReleasePhysicalPage(MMPFN *Pfn);

OS_STATUS MmAllocateVirtualPagesArenaEx(
    MM_ARENA *Arena,
    VOID *AddressHint,
    SIZE_T NumPages,
    UINT Flags,
    UINT Protection,
    VOID **AddressOut
);

FORCE_INLINE OS_STATUS MmReserveKernelVirtualPages(VOID *Address, SIZE_T NumPages) {
    VOID *Garbage;
    return MmAllocateVirtualPagesArenaEx(MmKernelArena, Address, NumPages, MM_ALLOC_FIXED | MM_ALLOC_UNBACKED, 0, &Garbage);
}

FORCE_INLINE VOID* MmAllocateVirtualPagesArena(
    MM_ARENA *Arena,
    VOID *AddressHint,
    SIZE_T NumPages,
    UINT Flags,
    UINT Protection
)
{
    VOID *Output;
    OS_STATUS Status = MmAllocateVirtualPagesArenaEx(Arena, AddressHint, NumPages, Flags, Protection, &Output);

    if (!OS_SUCCESS(Status)) {
        return NULL;
    }

    return Output;
}

OS_STATUS MmFreeVirtualPagesArena(
    MM_ARENA *Arena,
    VOID *Address,
    SIZE_T NumPages
);

#endif
