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
#include <ion/ke/mm/pool.h>
#include <ion/ke/mm/heap.h>

// Flags
#define MM_ALLOC_FIXED          0x01    // The allocation being requested is fixed at AddressHint, and should fail otherwise
#define MM_ALLOC_FIXED_REPLACE  0x02    // The allocation being requested is fixed at AddressHint, but should replace an existing mapping
#define MM_ALLOC_UNBACKED       0x04    // Do not back this virtual memory
#define MM_ALLOC_UNINIT         0x08    // (ignored if unbacked) This virtual memory is uninitialized, do not zero it

// Protection
#define MM_PROT_READ            0x01
#define MM_PROT_WRITE           0x02
#define MM_PROT_EXEC            0x04

// Heap flags
#define HEAP_FLAG_DEFAULT       0x00
#define HEAP_FLAG_UNINIT        0x01       // Do not initialize this memory with zeroes

// Pool flags
#define POOL_FLAG_DEFAULT       0x00
#define POOL_FLAG_UNINIT        0x01

FORCE_INLINE PFN_NUMBER MmConvertPhysToPfn(UINT_PTR Physical) {
    return (PFN_NUMBER)(Physical >> PAGE_SHIFT);
}

FORCE_INLINE PFN_NUMBER MmGetPfnNumber(MMPFN *Pfn) {
    return (PFN_NUMBER)(((UINT_PTR)Pfn - (UINT_PTR)MmPfnDatabase) / sizeof(MMPFN));
}

FORCE_INLINE UINT_PTR MmConvertPfnToPhys(PFN_NUMBER PfnNumber) {
    return (UINT_PTR)(PfnNumber << PAGE_SHIFT);
}

FORCE_INLINE UINT_PTR MmPageAlignUp(UINT_PTR Address) {
    return ((Address + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1));
}

FORCE_INLINE UINT_PTR MmPageAlignDown(UINT_PTR Address) {
    return (Address & ~(PAGE_SIZE-1));
}

/* Mi: Memory initialization */
OS_STATUS MiInitializePfnDatabase();
OS_STATUS MiInitializeKernelArena();
OS_STATUS MiInitializePoolManager();
OS_STATUS MiInitializeKernelHeaps();
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
PFN_NUMBER MmAllocatePhysicalPageZeroed();
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

OS_STATUS MmFreeVirtualPagesArena(MM_ARENA *Arena, VOID *Address, SIZE_T NumPages);

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

// heap management routines
OS_STATUS MmCreateHeap(const CHAR *HeapName, MM_HEAP_TYPE Type, MM_TAG Tag, MM_HEAP **Output);
OS_STATUS MmDestroyHeap(MM_HEAP *Heap);
OS_STATUS MmAllocateHeap2(MM_HEAP *Heap, SIZE_T Size, UINT AllocFlags, MM_TAG Tag, VOID **Output);
OS_STATUS MmFreeHeap(VOID *Allocation);

FORCE_INLINE VOID* MmAllocateHeapWithTagEx(MM_HEAP *Heap, SIZE_T Size, UINT AllocFlags, MM_TAG Tag) {
    VOID *Output;
    OS_STATUS Status = MmAllocateHeap2(Heap, Size, AllocFlags, Tag, &Output);
    if (!OS_SUCCESS(Status)) {
        return NULL;
    } else {
        return Output;
    }
}

FORCE_INLINE VOID* MmAllocateHeapWithTag(MM_HEAP *Heap, SIZE_T Size, MM_TAG Tag) {
    return MmAllocateHeapWithTagEx(Heap, Size, HEAP_FLAG_DEFAULT, Tag);
}

FORCE_INLINE VOID* MmAllocateHeapEx(MM_HEAP *Heap, SIZE_T Size, UINT AllocFlags) {
    VOID *Output;
    OS_STATUS Status = MmAllocateHeap2(Heap, Size, AllocFlags, Heap->DefaultTag, &Output);
    if (!OS_SUCCESS(Status)) {
        return NULL;
    } else {
        return Output;
    }
}

FORCE_INLINE VOID* MmAllocateHeap(MM_HEAP *Heap, SIZE_T Size) {
    return MmAllocateHeapEx(Heap, Size, HEAP_FLAG_DEFAULT);
}

// pool management routines
OS_STATUS MmCreatePool(
    const CHAR* Name,
    MM_POOL_TYPE Type,
    SIZE_T ObjectSize,
    SIZE_T ObjectAlignment,
    MM_POOL **Output
);

OS_STATUS MmDestroyPool(MM_POOL *Heap);
OS_STATUS MmAllocatePool2(MM_POOL *Pool, UINT AllocFlags, VOID **Output);
OS_STATUS MmFreePool(MM_POOL *Pool, VOID *Allocation);

FORCE_INLINE VOID* MmAllocatePoolEx(MM_POOL *Pool, UINT AllocFlags) {
    VOID *Output;
    OS_STATUS Status = MmAllocatePool2(Pool, AllocFlags, &Output);
    if (!OS_SUCCESS(Status)) {
        return NULL;
    }  else {
        return Output;
    }
}

FORCE_INLINE VOID* MmAllocatePool(MM_POOL *Pool) {
    return MmAllocatePoolEx(Pool, POOL_FLAG_DEFAULT);
}

#endif
