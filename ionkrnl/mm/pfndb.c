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

KSPIN_LOCK MmPfndbLock;
MMPFN *MmPfnDatabase = (MMPFN*)MM_PFNDB_ADDR;

static MMPFN *MmpFreeListHead = NULL;
static MMPFN *MmpFreeListTail = NULL;

// Convert to an HHDM address
UINT_PTR MiConvertToHhdm(UINT_PTR Address) {
    return KeLoaderParameterBlock->HhdmRegion + Address;
}

// Allocate a physical page
PFN_NUMBER MmAllocatePhysicalPage() {
    KIRQL OldIrql;
    KeAcquireSpinLock(&MmPfndbLock, &OldIrql);

    if (MmpFreeListHead == MmpFreeListTail) {
        KeBugCheckWithMessage("Out of available memory in freelist");
    }

    ASSERT(MmpFreeListHead->Alloc.Next != PFN_INVALID);

    MMPFN *Saved = MmpFreeListHead;
    MmpFreeListHead = &MmPfnDatabase[MmpFreeListHead->Alloc.Next];

    KeReleaseSpinLock(&MmPfndbLock, OldIrql);
    return MmGetPfnNumber(Saved);
}

// Allocate a page from the memory map
UINT_PTR MiAllocatePageFromMemoryMap() {
    for (size_t i = 0; i < KeLoaderParameterBlock->NrMemoryRegions; i++) {
        LOADER_MEMORY_REGION* Region = &KeLoaderParameterBlock->MemoryRegions[i];
        
        if (Region->Length > PAGE_SIZE) {
            UINT_PTR Saved = Region->Base;
            Region->Length -= PAGE_SIZE;
            Region->Base += PAGE_SIZE;
            return Saved;
        }
    }

    KeBugCheckWithMessage("MiAllocatePageFromMemoryMap had no available memory to allocate from");
    return 0x0;
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

        for (UINT_PTR j = Region->Base; j < Region->Base + Region->Length; j += PAGE_SIZE) {
            PFN_NUMBER PfnNumber = MmConvertPhysToPfn(j);
            MMPFN *Pfn = &MmPfnDatabase[PfnNumber];

            if (Region->Type != LOADER_MEMORY_AVAIL) {
                Pfn->Type = MM_PFN_ALLOCATED;
                Pfn->RefCount = 1;
                continue;
            }

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

    DbgPrint("MiInitializePfnDatabase finished\n");
    DbgPrint("MmAllocatePhysicalPage returned PFN %zu\n", MmAllocatePhysicalPage());
    return STATUS_SUCCESS;
}
