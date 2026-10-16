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
#include <ion/ke/mm.h>
#include <ion/third_party/limine.h>

extern UINT8 __ImageBase;
extern VOID MiMapPageAtAddressEarly(UINT_PTR Address);

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

volatile struct limine_kernel_file_request KeLimineKernelFileRequest = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .response = NULL,
    .revision = 0,
};

volatile struct limine_module_request KeLimineModuleRequest = {
    .id = LIMINE_MODULE_REQUEST,
    .response = NULL,
    .revision = 0
};

/* HHDM */
#define TO_HHDM(x) (VOID*)(KeLimineHhdmRequest.response->offset + (x))

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

    ASSERT(KeLimineMemmapRequest.response != NULL);
    ASSERT(KeLimineModuleRequest.response != NULL);

    // Reserve the storage before LPB memory map creation
    struct limine_memmap_response *MemMap = KeLimineMemmapRequest.response;
    LOADER_MEMORY_REGION *Regions = TO_HHDM(KeAllocateFromMemmap(MemMap->entry_count * sizeof(LOADER_MEMORY_REGION)));

    struct limine_module_response *Modules = KeLimineModuleRequest.response;
    LOADER_MODULE *LpbModules = NULL;
    if (Modules->module_count != 0) {
        LpbModules = TO_HHDM(KeAllocateFromMemmap(Modules->module_count * sizeof(LOADER_MODULE)));
    }

    struct limine_framebuffer_response *FramebufferResponse = KeLimineFramebufferRequest.response;
    LOADER_FRAMEBUFFER *LpbFramebuffers = NULL;
    if (FramebufferResponse != NULL && FramebufferResponse->framebuffer_count != 0) {
        LpbFramebuffers = TO_HHDM(KeAllocateFromMemmap(
            FramebufferResponse->framebuffer_count * sizeof(LOADER_FRAMEBUFFER)));
    }

    // Build the LPB memory map after all early allocations are reserved.
    for (SIZE_T i = 0; i < MemMap->entry_count; i++) {
        struct limine_memmap_entry *Entry = MemMap->entries[i];
        
        DbgPrint("Limine memory entry: %p - %p (%d)\n", Entry->base, Entry->base + Entry->length, Entry->type);
        Regions[i].Base = Entry->base;
        Regions[i].Length = Entry->length;
        Regions[i].Type = KeConvertLimineType(Entry->type);
    }

    KeLoaderParameterBlock->MemoryRegions = Regions;
    KeLoaderParameterBlock->NrMemoryRegions = MemMap->entry_count;

    // Add HHDM
    KeLoaderParameterBlock->HhdmRegion = KeLimineHhdmRequest.response->offset;
    DbgPrint("HhdmRegion @ %p\n", KeLoaderParameterBlock->HhdmRegion);

    // Add modules
    for (SIZE_T i = 0; i < Modules->module_count; i++) {
        LOADER_MODULE *LpbMod = &LpbModules[i];
        struct limine_file *BootMod = Modules->modules[i];

        LpbMod->Base = (UINT_PTR)BootMod->address;
        LpbMod->Cmdline = BootMod->cmdline;
        LpbMod->Path = BootMod->path;
        LpbMod->Size =  BootMod->size;
    }

    KeLoaderParameterBlock->LoadedModules = LpbModules;
    KeLoaderParameterBlock->NrLoadedModules = Modules->module_count;

    if (FramebufferResponse != NULL) {
        for (SIZE_T i = 0; i < FramebufferResponse->framebuffer_count; i++) {
            struct limine_framebuffer *BootFramebuffer = FramebufferResponse->framebuffers[i];
            LOADER_FRAMEBUFFER *Framebuffer = &LpbFramebuffers[i];

            Framebuffer->Address = (UINT_PTR)BootFramebuffer->address;
            Framebuffer->Width = BootFramebuffer->width;
            Framebuffer->Height = BootFramebuffer->height;
            Framebuffer->Pitch = BootFramebuffer->pitch;
            Framebuffer->Size = BootFramebuffer->pitch * BootFramebuffer->height;
            Framebuffer->Bpp = BootFramebuffer->bpp;
            Framebuffer->MemoryModel = BootFramebuffer->memory_model;
            Framebuffer->RedMaskSize = BootFramebuffer->red_mask_size;
            Framebuffer->RedMaskShift = BootFramebuffer->red_mask_shift;
            Framebuffer->GreenMaskSize = BootFramebuffer->green_mask_size;
            Framebuffer->GreenMaskShift = BootFramebuffer->green_mask_shift;
            Framebuffer->BlueMaskSize = BootFramebuffer->blue_mask_size;
            Framebuffer->BlueMaskShift = BootFramebuffer->blue_mask_shift;
        }
        KeLoaderParameterBlock->NrFramebuffers = FramebufferResponse->framebuffer_count;
    }

    KeLoaderParameterBlock->Framebuffers = LpbFramebuffers;
}

VOID KeSystemStartup() {
    DbgInit();

    DbgPrint("Ion OS kernel\n");

    /* Prepare the LPB */
    KeInitLoaderParameterBlock();

    /* Go */
    return KeInitialize();
}
