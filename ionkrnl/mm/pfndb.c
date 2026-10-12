/**
 * @file ionkrnl/mm/pfndb.c
 * @brief Ion's physical memory manager
 * 
 * Ion's PMM uses a freelist (it does not support multi-page allocations yet)
 * for O(1) alloc/freeing
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ion.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>

extern VOID MiMapPageAtAddressEarly(UINT_PTR Address);

/* DEBUG enable */
#define MM_DEBUG 1

#if MM_DEBUG
#define DBG_ASSERT assert
#define DBG(...) DbgPrint(__VA_ARGS__)
#else
#define DBG_ASSERT(x) ((void)0)
#define DBG(...) ((void)0)
#endif

/* Lock */
KSPIN_LOCK MmPfndbLock;

/* Database */
MMPFN *MmPfnDatabase = (MMPFN*)MM_PFNDB_ADDR;

/* Free list */
static MMPFN *MmpFreeListHead = NULL;
static MMPFN *MmpFreeListTail = NULL;

// Add to a freelist
static VOID MmpAddToFreelist(MMPFN **Head, MMPFN **Tail, MMPFN *Target) {
    MMPFN *Existing = *Tail;
    Existing->Alloc.Next = MmGetPfnNumber(Target);
    Target->Alloc.Prev = MmGetPfnNumber(Existing);
    *Tail = Target;
}

// Remove from a freelist
static MMPFN* MmpRemoveFromFreelist(MMPFN **Head, MMPFN **Tail) {
    MMPFN *Existing = *Head;
    if (Existing == *Tail) {
        return NULL;
    }

    MMPFN *Next = &MmPfnDatabase[Existing->Alloc.Next];
    Next->Alloc.Prev = PFN_INVALID;
    *Head = Next;

    return Existing;
}

// Convert to an HHDM address
UINT_PTR MiConvertToHhdm(UINT_PTR Address) {
    return KeLoaderParameterBlock->HhdmRegion + Address;
}

// Allocate a page from the memory map
UINT_PTR MiAllocatePageFromMemoryMap() {
    for (size_t i = 0; i < KeLoaderParameterBlock->NrMemoryRegions; i++) {
        LOADER_MEMORY_REGION* Region = &KeLoaderParameterBlock->MemoryRegions[i];
        
        if (Region->Type == LOADER_MEMORY_AVAIL && Region->Length > PAGE_SIZE) {
            UINT_PTR Saved = Region->Base;
            Region->Length -= PAGE_SIZE;
            Region->Base += PAGE_SIZE;
            return Saved;
        }
    }

    KeBugCheckWithMessage("MiAllocatePageFromMemoryMap had no available memory to allocate from");
    return 0x0;
}

// Reference a physical page
VOID MmHoldPhysicalPage(MMPFN *Pfn) {
    __atomic_add_fetch(&Pfn->RefCount, 1, __ATOMIC_SEQ_CST);
}

// Dereference a physical page (will free if refcount hits 0)
VOID MmReleasePhysicalPage(MMPFN *Pfn) {
    if (__atomic_sub_fetch(&Pfn->RefCount, 1, __ATOMIC_SEQ_CST) != 0) {
        return;
    }

    KIRQL OldIrql;
    KeAcquireSpinLock(&MmPfndbLock, &OldIrql);
    Pfn->Type = MM_PFN_FREE;
    MmpAddToFreelist(&MmpFreeListHead, &MmpFreeListTail, Pfn);
    KeReleaseSpinLock(&MmPfndbLock, OldIrql);
}

// Allocate a physical page
PFN_NUMBER MmAllocatePhysicalPage() {
    KIRQL OldIrql;
    KeAcquireSpinLock(&MmPfndbLock, &OldIrql);

    MMPFN *Target = MmpRemoveFromFreelist(&MmpFreeListHead, &MmpFreeListTail);
    if (Target == NULL) {
        KeBugCheckWithMessage("Out of available memory in freelist");
    }

    MmHoldPhysicalPage(Target);
    Target->Type = MM_PFN_ALLOCATED;

    KeReleaseSpinLock(&MmPfndbLock, OldIrql);
    return MmGetPfnNumber(Target);
}

// Free a physical page
VOID MmFreePhysicalPage(PFN_NUMBER PfnNumber) {
    MmReleasePhysicalPage(&MmPfnDatabase[PfnNumber]);
}

// Initialize the PFN database
OS_STATUS MiInitializePfnDatabase() {
    KeInitializeSpinLock(&MmPfndbLock);

    // The PFNDB is located at MM_PFNDB_BASE, and it must be mapped in its entirety for
    // all available regions
    for (int i = 0; i < KeLoaderParameterBlock->NrMemoryRegions; i++) {
        LOADER_MEMORY_REGION* Region = &KeLoaderParameterBlock->MemoryRegions[i];
        if (Region->Type != LOADER_MEMORY_AVAIL && Region->Type != LOADER_MEMORY_KERNEL) {
            continue;
        }

        PFN_NUMBER Start = MmConvertPhysToPfn(Region->Base);
        PFN_NUMBER End = MmConvertPhysToPfn(Region->Base + Region->Length);

        UINT_PTR First = MM_PFNDB_ADDR + Start * sizeof(MMPFN);
        UINT_PTR Last = MM_PFNDB_ADDR + End * sizeof(MMPFN);

        First &= ~(UINT_PTR)(PAGE_SIZE - 1);
        Last = (Last + PAGE_SIZE - 1) & ~(UINT_PTR)(PAGE_SIZE - 1);

        for (UINT_PTR Base = First; Base < Last; Base += PAGE_SIZE) {
            MiMapPageAtAddressEarly(Base);
            RtlZeroMemory((VOID*)Base, PAGE_SIZE);
        }
    }

    // Now the database must be initialized
    MMPFN *LastPfn = NULL;
    PFN_NUMBER LastPfnNumber;
    for (int i = 0; i < KeLoaderParameterBlock->NrMemoryRegions; i++) {
        LOADER_MEMORY_REGION* Region = &KeLoaderParameterBlock->MemoryRegions[i];
        if (Region->Type != LOADER_MEMORY_AVAIL && Region->Type != LOADER_MEMORY_KERNEL) {
            continue;
        }

        DBG("Initializing valid memory region %p - %p\n", Region->Base, Region->Base + Region->Length);

        for (UINT_PTR j = Region->Base; j < Region->Base + Region->Length; j += PAGE_SIZE) {
            PFN_NUMBER PfnNumber = MmConvertPhysToPfn(j);
            MMPFN *Pfn = &MmPfnDatabase[PfnNumber];

            if (Region->Type != LOADER_MEMORY_AVAIL) {
                Pfn->Type = MM_PFN_ALLOCATED;
                Pfn->RefCount = 1;
                continue;
            } else {
                Pfn->Type = MM_PFN_FREE;
                Pfn->RefCount = 0;

                if (LastPfn) {
                    LastPfn->Alloc.Next = PfnNumber;
                    Pfn->Alloc.Prev = LastPfnNumber;
                } else {
                    Pfn->Alloc.Prev = PFN_INVALID;
                }

                Pfn->Alloc.Next = PFN_INVALID;
                LastPfn = Pfn;
                LastPfnNumber = PfnNumber;

                if (!MmpFreeListHead) MmpFreeListHead = Pfn;
                MmpFreeListTail = Pfn;
            }
        }
    }

    DbgPrint("MiInitializePfnDatabase finished\n");
    return STATUS_SUCCESS;
}
