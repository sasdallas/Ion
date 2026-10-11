/**
 * @file ionkrnl/ke/x86_64/init.c
 * @brief x86_64 Ke initialization code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ion.h>
#include <ion/ke/ke.h>
#include <ion/third_party/limine.h>

#if LIMINE_API_REVISION >= 3
    #define STRING_OR_CMDLINE string
#else
    #define STRING_OR_CMDLINE cmdline
#endif

#if LIMINE_API_REVISION >= 2
    #define __LIMINE_MEMMAP_KERNEL_AND_MODULES LIMINE_MEMMAP_EXECUTABLE_AND_MODULES
#else
    #define __LIMINE_MEMMAP_KERNEL_AND_MODULES LIMINE_MEMMAP_KERNEL_AND_MODULES
#endif

volatile struct limine_framebuffer_request KeLimineFramebufferRequest = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .response = NULL,
    .revision = 0,  
};

volatile struct limine_memmap_request KeLimineMemmapRequest = {
    .id = LIMINE_MEMMAP_REQUEST,
    .response = NULL,
    .revision = 0
};

volatile struct limine_hhdm_request KeLimineHhdmRequest = {
    .id = LIMINE_HHDM_REQUEST,
    .response = NULL,
    .revision = 0
};

/* Loader block */
static LOADER_PARAMETER_BLOCK __KeLoaderParameterBlock;

static VOID* KeAllocateFromMemmap(SIZE_T Size) {
    /* round up size */
    Size = (Size + 0xFFF) & ~0xFFF;

    struct limine_memmap_response *MemMap = KeLimineMemmapRequest.response;
    for (SIZE_T i = 0; i < MemMap->entry_count; i++) {
        struct limine_memmap_entry *Entry = MemMap->entries[i];
        if (Entry->type == LIMINE_MEMMAP_USABLE && Entry->length >= Size) {
            VOID *OldBase = (VOID*)Entry->base;
            Entry->base += Size;
            Entry->length -= Size;
            return OldBase;
        }
    }

    ASSERT(0);
    return NULL;
}

FORCE_INLINE LOADER_MEMORY_TYPE KeConvertLimineType(UINT64 Type) {
    switch (Type) {
        case LIMINE_MEMMAP_USABLE:
            return LOADER_MEMORY_AVAIL;
        case LIMINE_MEMMAP_BAD_MEMORY:
            return LOADER_MEMORY_BAD;
        case LIMINE_MEMMAP_RESERVED:
            return LOADER_MEMORY_RESERVED;
        case LIMINE_MEMMAP_ACPI_NVS:
            return LOADER_MEMORY_ACPI_NVS;
        case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            return LOADER_MEMORY_ACPI_RECLAIM;
        case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            return LOADER_MEMORY_RESERVED; /* TODO: reclaim bootloader memory */
        case LIMINE_MEMMAP_FRAMEBUFFER:
            return LOADER_MEMORY_RESERVED;
        case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            return LOADER_MEMORY_KERNEL;
        default:
            ASSERT(0 && "KeConvertLimineType encountered bad memory type!");
    }
}

VOID KeInitLoaderParameterBlock() {
    KeLoaderParameterBlock = &__KeLoaderParameterBlock;

    struct limine_memmap_response *MemMap = KeLimineMemmapRequest.response;
    LOADER_MEMORY_REGION *Regions = KeAllocateFromMemmap(MemMap->entry_count * sizeof(LOADER_MEMORY_REGION));
    for (SIZE_T i = 0; i < MemMap->entry_count; i++) {
        struct limine_memmap_entry *Entry = MemMap->entries[i];
        
        DbgPrint("Limine memory entry: %p - %p (%d)\n", Entry->base, Entry->base + Entry->length, Entry->type);
        Regions[i].Base = Entry->base;
        Regions[i].Length = Entry->length;
        Regions[i].Type = KeConvertLimineType(Entry->type);
    }

    KeLoaderParameterBlock->MemoryRegions = Regions;
    KeLoaderParameterBlock->NrMemoryRegions = MemMap->entry_count;

    /* HHDM */
    KeLoaderParameterBlock->HhdmRegion = KeLimineHhdmRequest.response->offset;

    DbgPrint("HhdmRegion @ %p\n", KeLoaderParameterBlock->HhdmRegion);
}

VOID KeSystemStartup() {
    DbgInit();

    DbgPrint("Ion OS kernel\n");

    /* Prepare the LPB */
    KeInitLoaderParameterBlock();

    /* Go */
    return KeInitialize();
}
