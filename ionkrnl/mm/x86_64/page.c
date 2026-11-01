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

typedef struct { MMPTE Entries[512]; } PML;

/* Kernel address space */
static MM_ADDRESS_SPACE __MmKernelAddressSpace;
MM_ADDRESS_SPACE *MmKernelAddressSpace = &__MmKernelAddressSpace;

// WB, WT, UC-, UC, WB, WT, UC-, UC
#define MM_IA32_PAT 0x0007040600070406ULL

static VOID MiInitalizePat() {
    BOOLEAN Restore = KeSetInterruptState(FALSE);
    
    // disable NW and enable CD on CR0 to disable caching
    UINT64 Cr0;
    asm volatile ("mov %%cr0, %0" : "=r"(Cr0));
    UINT64 UncachedCr0 = (Cr0 | BIT(30)) & ~BIT(29);
    asm volatile ("mov %0, %%cr0" :: "r"(UncachedCr0) : "memory");
    asm volatile ("wbinvd" ::: "memory");

    // prepare the PAT
    UINT_PTR Cr3 = (UINT_PTR)MmuCurrentPageTable();
    asm volatile ("mov %0, %%cr3" :: "r"(Cr3) : "memory");
    asm volatile ("wrmsr" :: "c"(0x277), "a"((UINT32)MM_IA32_PAT), "d"((UINT32)(MM_IA32_PAT >> 32)) : "memory");

    // reset cr3 + cr0
    asm volatile ("wbinvd" ::: "memory");
    asm volatile ("mov %0, %%cr3" :: "r"(Cr3) : "memory");
    asm volatile ("mov %0, %%cr0" :: "r"(Cr0) : "memory");
    KeSetInterruptState(Restore);
}

OS_STATUS MiInitializeKernelAddressSpace() {
    __MmKernelAddressSpace.PageTable = MmuCurrentPageTable();
    KeInitializeSpinLock(&__MmKernelAddressSpace.Lock);
    return STATUS_SUCCESS;
}

// Function to map a page at an address for early allocation
VOID MiMapPageAtAddressEarly(UINT_PTR Address) {
    MM_PAGETBL *PageTable = MmuCurrentPageTable();
    PML* Pml = (PML*)MiConvertToHhdm((UINT_PTR)PageTable);

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
    
        asm volatile ("invlpg (%0)" :: "r"(Address) : "memory");
    }
}

static MMPTE* MmuGetPage(MM_ADDRESS_SPACE *AddressSpace, UINT_PTR VirtualAddress, BOOLEAN AllocatePage) {
    // TODO: Use recursive paging
    PML* Pml = (PML*)MiConvertToHhdm((UINT_PTR)AddressSpace->PageTable);

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

OS_STATUS MmuMapPageEx(MM_ADDRESS_SPACE *AddressSpace, UINT_PTR VirtualAddress, UINT_PTR PhysicalAddress, UINT Protection, MM_CACHING_TYPE CacheType) {
    KIRQL Irql;
    KeAcquireSpinLock(&AddressSpace->Lock, &Irql);

    MMPTE *Pte = MmuGetPage(AddressSpace, VirtualAddress, TRUE);

    MMPTE PteFlags = MM_PTE_PRESENT;
    if (Protection & MM_PROT_WRITE) PteFlags |= MM_PTE_WRITABLE;
    if ((Protection & MM_PROT_EXEC) == 0) PteFlags |= MM_PTE_NX;
    if (CacheType == MmCacheUncached) PteFlags |= MM_PTE_PWT | MM_PTE_PCD;

    *(Pte) = (PhysicalAddress & MM_PTE_ADDR_MASK) | PteFlags;

    if (AddressSpace->PageTable == MmuCurrentPageTable()) {
        asm volatile ("invlpg (%0)" :: "r"(VirtualAddress) : "memory");
    }

    KeReleaseSpinLock(&AddressSpace->Lock, Irql);
    return STATUS_SUCCESS;
}

BOOLEAN MmuUnmapPage(MM_ADDRESS_SPACE *AddressSpace, UINT_PTR VirtualAddress, UINT_PTR *PhysicalAddress) {
    KIRQL Irql;
    KeAcquireSpinLock(&AddressSpace->Lock, &Irql);
    MMPTE *Pte = MmuGetPage(AddressSpace, VirtualAddress, FALSE);
    if (Pte == NULL || !(*Pte & MM_PTE_PRESENT)) {
        KeReleaseSpinLock(&AddressSpace->Lock, Irql);
        return FALSE;
    }

    if (PhysicalAddress != NULL) {
        *PhysicalAddress = *Pte & MM_PTE_ADDR_MASK;
    }

    // zero the PTE
    *Pte = 0;

    if (AddressSpace->PageTable == MmuCurrentPageTable()) {
        asm volatile ("invlpg (%0)" :: "r"(VirtualAddress) : "memory");
    }

    KeReleaseSpinLock(&AddressSpace->Lock, Irql);
    return TRUE;
}

UINT_PTR MmuGetPhysicalAddress(MM_ADDRESS_SPACE *AddressSpace, UINT_PTR VirtualAddress) {
    KIRQL Irql;
    KeAcquireSpinLock(&AddressSpace->Lock, &Irql);

    PML *Pml = (PML*)MiConvertToHhdm((UINT_PTR)AddressSpace->PageTable);
    for (INT Level = 3; Level > 0; Level--) {
        UINT_PTR Index = (VirtualAddress >> (PAGE_SHIFT + Level * 9)) & 0x1FF;
        MMPTE Entry = Pml->Entries[Index];
        if ((Entry & MM_PTE_PRESENT) == 0) {
            KeReleaseSpinLock(&AddressSpace->Lock, Irql);
            return 0;
        }

        if (Entry & MM_PTE_PS) {
            UINT_PTR PageSize = 1ULL << (PAGE_SHIFT + Level * 9);
            UINT_PTR Physical = (Entry & MM_PTE_ADDR_MASK & ~(PageSize - 1)) | (VirtualAddress & (PageSize - 1));
            KeReleaseSpinLock(&AddressSpace->Lock, Irql);
            return Physical;
        }

        Pml = (PML*)MiConvertToHhdm(Entry & MM_PTE_ADDR_MASK);
    }

    MMPTE Entry = Pml->Entries[(VirtualAddress >> PAGE_SHIFT) & 0x1FF];
    if ((Entry & MM_PTE_PRESENT) == 0) {
        KeReleaseSpinLock(&AddressSpace->Lock, Irql);
        return 0;
    }

    UINT_PTR Physical = (Entry & MM_PTE_ADDR_MASK) | (VirtualAddress & (PAGE_SIZE - 1));
    KeReleaseSpinLock(&AddressSpace->Lock, Irql);
    return Physical;
}

OS_STATUS MiMapFramebuffers() {
    for (SIZE_T i = 0; i < KeLoaderParameterBlock->NrFramebuffers; i++) {
        LOADER_FRAMEBUFFER *Framebuffer = &KeLoaderParameterBlock->Framebuffers[i];
        UINT_PTR Physical = MmuGetPhysicalAddress(MmKernelAddressSpace, Framebuffer->Address);
        VOID *Mapping = MmMapIoSpace(Physical, Framebuffer->Size, MmCacheUncached);
        if (Mapping == NULL) return STATUS_NO_MEMORY;
        Framebuffer->Address = (UINT_PTR)Mapping;
    }

    return STATUS_SUCCESS;
}

VOID MmuSetAddressSpace(MM_ADDRESS_SPACE *AddressSpace) {
    asm volatile ("movq %0, %%cr3" :: "r"(AddressSpace->PageTable) : "memory");
}

VOID MmuInvalidateRange(MM_ADDRESS_SPACE *AddressSpace, UINT_PTR Start, SIZE_T NumPages) {
    if (AddressSpace->PageTable != MmuCurrentPageTable()) return;
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
