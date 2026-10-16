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
static MMPFN *MmpZeroFreeListHead = NULL;
static MMPFN *MmpZeroFreeListTail = NULL;

/* Stats */
SIZE_T MmTotalPhysicalMemory = 0;
SIZE_T MmUsedPhysicalMemory = 0;

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

// Add to a freelist
static VOID MmAddToFreelist(MMPFN **Head, MMPFN **Tail, MMPFN *Target) {
    Target->Alloc.Next = PFN_INVALID;
    Target->Alloc.Prev = *Tail ? MmGetPfnNumber(*Tail) : PFN_INVALID;
    if (*Tail) {
        (*Tail)->Alloc.Next = MmGetPfnNumber(Target);
    } else {
        *Head = Target;
    }
    *Tail = Target;
}

// Remove from a freelist
static MMPFN* MmPopFromFreelist(MMPFN **Head, MMPFN **Tail) {
    MMPFN *Existing = *Head;
    if (Existing == NULL) {
        return NULL;
    }

    if (Existing->Alloc.Next == PFN_INVALID) {
        *Head = NULL;
        *Tail = NULL;
    } else {
        MMPFN *Next = &MmPfnDatabase[Existing->Alloc.Next];
        Next->Alloc.Prev = PFN_INVALID;
        *Head = Next;
    }

    Existing->Alloc.Next = PFN_INVALID;
    Existing->Alloc.Prev = PFN_INVALID;

    return Existing;
}

static VOID MmRemoveFromFreelist(MMPFN **Head, MMPFN **Tail, MMPFN *Target) {
    if (Target->Alloc.Next != PFN_INVALID) {
        MMPFN *Next = &MmPfnDatabase[Target->Alloc.Next];
        Next->Alloc.Prev = Target->Alloc.Prev;
    }

    if (Target->Alloc.Prev != PFN_INVALID) {
        MMPFN *Prev = &MmPfnDatabase[Target->Alloc.Prev];
        Prev->Alloc.Next = Target->Alloc.Next;
    }

    if (*Head == Target) {
        *Head = Target->Alloc.Next == PFN_INVALID ? NULL : &MmPfnDatabase[Target->Alloc.Next];
    }

    if (*Tail == Target) {
        *Tail = Target->Alloc.Prev == PFN_INVALID ? NULL : &MmPfnDatabase[Target->Alloc.Prev];
    }

    Target->Alloc.Next = PFN_INVALID;
    Target->Alloc.Prev = PFN_INVALID;
}

// Make physical page zeroed
static VOID MmMakePhysicalPageZeroed(MMPFN *Pfn) {
    if (Pfn->Type == PFN_TYPE_ZEROED) {
        return;
    }

    KIRQL Irql;
    KeAcquireSpinLock(&MmPfndbLock, &Irql);

    // Zero the pfn
    PFN_NUMBER PfnNumber = MmGetPfnNumber(Pfn);
    UINT_PTR Hhdm = MiConvertToHhdm(MmConvertPfnToPhys(PfnNumber));
    RtlZeroMemory((VOID*)Hhdm, PAGE_SIZE);

    // Move it to a different freelist
    MmRemoveFromFreelist(&MmpFreeListHead, &MmpFreeListTail, Pfn);

    Pfn->Type = PFN_TYPE_ZEROED;
    MmAddToFreelist(&MmpZeroFreeListHead, &MmpZeroFreeListTail, Pfn);

    KeReleaseSpinLock(&MmPfndbLock, Irql);
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
    Pfn->Type = PFN_TYPE_FREE;
    MmAddToFreelist(&MmpFreeListHead, &MmpFreeListTail, Pfn);
    KeReleaseSpinLock(&MmPfndbLock, OldIrql);
    
    // Update statistics
    __atomic_sub_fetch(&MmUsedPhysicalMemory, PAGE_SIZE, __ATOMIC_RELAXED);
}

// Allocate a physical page
PFN_NUMBER MmAllocatePhysicalPage() {
    KIRQL OldIrql;
    KeAcquireSpinLock(&MmPfndbLock, &OldIrql);

    MMPFN *Target = MmPopFromFreelist(&MmpFreeListHead, &MmpFreeListTail);
    if (Target == NULL) {
        // Try taking from the zero list
        Target = MmPopFromFreelist(&MmpZeroFreeListHead, &MmpZeroFreeListTail);

        if (Target == NULL) {
            // TODO
            KeBugCheckWithMessage("Out of memory in all PFN freelists!");
        }
    }

    MmHoldPhysicalPage(Target);
    Target->Type = PFN_TYPE_ALLOCATED;

    KeReleaseSpinLock(&MmPfndbLock, OldIrql);

    // Update statistics
    __atomic_add_fetch(&MmUsedPhysicalMemory, PAGE_SIZE, __ATOMIC_RELAXED);

    return MmGetPfnNumber(Target);
}

// Allocate a zeroed physical page
PFN_NUMBER MmAllocatePhysicalPageZeroed() {
    KIRQL OldIrql;
    KeAcquireSpinLock(&MmPfndbLock, &OldIrql);

    MMPFN *Target = MmPopFromFreelist(&MmpZeroFreeListHead, &MmpZeroFreeListTail);
    if (Target == NULL) {
        // Try taking from the nonzero list
        Target = MmPopFromFreelist(&MmpFreeListHead, &MmpFreeListTail);

        if (Target == NULL) {
            // TODO
            KeBugCheckWithMessage("Out of memory in all PFN freelists!");
        }
    }

    MmHoldPhysicalPage(Target);

    BOOLEAN ShouldZero = Target->Type != PFN_TYPE_ZEROED;
    Target->Type = PFN_TYPE_ALLOCATED;

    KeReleaseSpinLock(&MmPfndbLock, OldIrql);

    // If needed, zero the physical page too
    if (ShouldZero == TRUE) {
        UINT_PTR Hhdm = MiConvertToHhdm(MmConvertPfnToPhys(MmGetPfnNumber(Target)));
        RtlZeroMemory((VOID*)Hhdm, PAGE_SIZE);
    }

    // Update statistics
    __atomic_add_fetch(&MmUsedPhysicalMemory, PAGE_SIZE, __ATOMIC_RELAXED);

    return MmGetPfnNumber(Target);
}

// Free a physical page
VOID MmFreePhysicalPage(PFN_NUMBER PfnNumber) {
    MmReleasePhysicalPage(&MmPfnDatabase[PfnNumber]);
}

// Zero some pages
static VOID MiZeroSomePages() {
    for (int i = 0; i < 200; i++) {
        PFN_NUMBER PfnNumber = MmAllocatePhysicalPage();
        ASSERT(PfnNumber != PFN_INVALID);

        UINT_PTR Hhdm = MiConvertToHhdm(MmConvertPfnToPhys(PfnNumber));
        RtlZeroMemory((VOID*)Hhdm, PAGE_SIZE);

        MMPFN *Pfn = &MmPfnDatabase[PfnNumber];
        Pfn->Type = PFN_TYPE_ZEROED;
        Pfn->RefCount = 0;

        MmAddToFreelist(&MmpZeroFreeListHead, &MmpZeroFreeListTail, Pfn);
    }
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
                Pfn->Type = PFN_TYPE_ALLOCATED;
                Pfn->RefCount = 1;

                MmUsedPhysicalMemory += PAGE_SIZE;
                continue;
            } else {
                Pfn->Type = PFN_TYPE_FREE;
                Pfn->RefCount = 0;

                MmAddToFreelist(&MmpFreeListHead, &MmpFreeListTail, Pfn);
            }

            MmTotalPhysicalMemory += PAGE_SIZE;
        }
    }

    // If we clean out a few pages before initialization it helps greatly
    // since most of the pages will need to be zeroed by the heap anyways
    MiZeroSomePages();

    DbgPrint("MiInitializePfnDatabase finished\n");
    return STATUS_SUCCESS;
}
