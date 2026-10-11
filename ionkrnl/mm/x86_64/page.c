/**
 * @file ionkrnl/mm/x86_64/page.c
 * @brief x86_64 PTE/page manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/rtl.h>

extern UINT8 __ImageBase, __ImageEnd;

typedef struct { MMPTE Entries[512]; } PACKED PML;

// Function to map a page at an address for early allocation
VOID MiMapPageAtAddressEarly(UINT_PTR Address) {
    UINT_PTR PageTable = MmuCurrentPageTable();
    PML* Pml = (PML*)MiConvertToHhdm(PageTable);

    UINT_PTR Levels[4] = {
        (Address >> 39) & 0x1FF, // PML4
        (Address >> 30) & 0x1FF, // PDPT
        (Address >> 21) & 0x1FF, // PD
        (Address >> 12) & 0x1FF, // PT
    };

    for (int i = 0; i < 3; i++) {
        MMPTE Ent = Pml->Entries[Levels[i]];
        if ((Ent & MM_PTE_PRESENT) == 0) {
            /* Allocate a new page */
            UINT_PTR NewPage = MiAllocatePageFromMemoryMap();

            UINT_PTR NewPageReal = MiConvertToHhdm(NewPage);
            RtlZeroMemory((VOID*)NewPageReal, PAGE_SIZE);

            Ent = NewPage | MM_PTE_PRESENT | MM_PTE_WRITABLE;
            Pml->Entries[Levels[i]] = Ent;
        }

        Pml = (PML*)MiConvertToHhdm(Ent & MM_PTE_ADDR_MASK);
    }

    /* We are at PT level */
    MMPTE *Pte = &Pml->Entries[Levels[3]];
    if ((*Pte & MM_PTE_PRESENT) == 0) {
        UINT_PTR NewPage = MiAllocatePageFromMemoryMap();
        *Pte = NewPage | MM_PTE_PRESENT | MM_PTE_WRITABLE;
    
        asm volatile ("invlpg %0" :: "m"(Address));
    }
}

static MMPTE* MmuGetPage(UINT_PTR PageTable, UINT_PTR VirtualAddress, BOOLEAN AllocatePage) {
    if (PageTable == 0x0) {
        PageTable = MmuCurrentPageTable();
    }

    // TODO: Use recursive paging
    PML* Pml = (PML*)MiConvertToHhdm(PageTable);

    UINT_PTR Levels[4] = {
        (VirtualAddress >> 39) & 0x1FF, // PML4
        (VirtualAddress >> 30) & 0x1FF, // PDPT
        (VirtualAddress >> 21) & 0x1FF, // PD
        (VirtualAddress >> 12) & 0x1FF, // PT
    };

    for (int i = 0; i < 3; i++) {
        MMPTE Ent = Pml->Entries[Levels[i]];
        ASSERT((Ent & MM_PTE_PS) == 0);
        if ((Ent & MM_PTE_PRESENT) == 0) {
            if (AllocatePage == FALSE) {
                return NULL;
            }

            /* Allocate a new page */
            UINT_PTR NewPage = MmConvertPfnToPhys(MmAllocatePhysicalPage());

            UINT_PTR NewPageReal = MiConvertToHhdm(NewPage);
            RtlZeroMemory((VOID*)NewPageReal, PAGE_SIZE);

            Ent = NewPage | MM_PTE_PRESENT | MM_PTE_WRITABLE | MM_PTE_USER;
            Pml->Entries[Levels[i]] = Ent;
        }

        Pml = (PML*)MiConvertToHhdm(Ent & MM_PTE_ADDR_MASK);
    }

    /* We are at PT level */
    return &Pml->Entries[Levels[3]];
}

VOID MmuMapPage(UINT_PTR PageTable, UINT_PTR VirtualAddress, UINT_PTR PhysicalAddress, UINT Protection) {
    MMPTE *Pte = MmuGetPage(PageTable, VirtualAddress, TRUE);
    UINT PteFlags = MM_PTE_PRESENT;
    if (Protection & MM_PROT_WRITE) PteFlags |= MM_PTE_WRITABLE;
    if ((Protection & MM_PROT_EXEC) == 0) PteFlags |= MM_PTE_NX;

    *(Pte) = (PhysicalAddress & MM_PTE_ADDR_MASK) | PteFlags;
}

VOID MmuUnmapPage(UINT_PTR PageTable, UINT_PTR VirtualAddress) {
    MMPTE *Pte = MmuGetPage(PageTable, VirtualAddress, FALSE);
    if (Pte == NULL) return;
    *Pte = 0;
}

UINT_PTR MmuGetPhysicalAddress(UINT_PTR PageTable, UINT_PTR VirtualAddress) {
    MMPTE *Pte = MmuGetPage(PageTable, VirtualAddress, FALSE);
    if (Pte == NULL) return 0x0;

    return (UINT_PTR)(*Pte & MM_PTE_ADDR_MASK);
}

VOID MmuInvalidateRange(UINT_PTR Start, SIZE_T NumPages) {
    if (NumPages > 16) {
        // reload cr3 instead, its faster
        asm volatile ("movq %%cr3, %%rax\n"
                      "movq %%rax, %%cr3" ::: "rax", "memory");
    } else {
        for (UINT_PTR i = Start; i < Start + (NumPages*PAGE_SIZE); i += PAGE_SIZE) {
            asm volatile ("invlpg (%0)" :: "r"(i) : "memory");
        }
    }
}

OS_STATUS MiUnmarkUsedMemory() {
    UINT_PTR ImageStart = (UINT_PTR)&__ImageBase;
    UINT_PTR ImageEnd = (UINT_PTR)&__ImageEnd;

    // Reserve the kernel image in memory
    OS_STATUS Status = MmReserveKernelVirtualPages((VOID*)ImageStart, (ImageEnd - ImageStart + PAGE_SIZE - 1) / PAGE_SIZE);
    if (!OS_SUCCESS(Status)) return Status;

    // Calculate the end of the HHDM region
    UINT_PTR MaxPhysicalEnd = 0;
    for (SIZE_T i = 0; i < KeLoaderParameterBlock->NrMemoryRegions; i++) {
        LOADER_MEMORY_REGION *Region = &KeLoaderParameterBlock->MemoryRegions[i];
        UINT_PTR End = Region->Base + Region->Length;
        if (End > MaxPhysicalEnd) MaxPhysicalEnd = End;
    }

    // Reserve the HHDM
    SIZE_T PhysicalPages = (MaxPhysicalEnd + PAGE_SIZE - 1) / PAGE_SIZE;
    Status = MmReserveKernelVirtualPages((VOID*)KeLoaderParameterBlock->HhdmRegion, PhysicalPages);
    if (!OS_SUCCESS(Status)) return Status;

    // Reserve the PFNDB
    SIZE_T PfndbBytes = PhysicalPages * sizeof(MMPFN);
    return MmReserveKernelVirtualPages((VOID*)MM_PFNDB_ADDR, (PfndbBytes + PAGE_SIZE - 1) / PAGE_SIZE);
}
