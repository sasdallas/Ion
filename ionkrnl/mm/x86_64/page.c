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
#include <ion/rtl.h>

typedef struct { MMPTE Entries[512]; } PACKED PML;

// Function to map a page at an address for early allocation
VOID MiMapPageAtAddressEarly(UINT_PTR Address) {
    UINT_PTR PageTable = MmCurrentPageTable();
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
