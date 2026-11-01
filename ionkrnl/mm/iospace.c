/**
 * @file ionkrnl/mm/iospace.c
 * @brief Kernel mappings for physical device memory
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/ion.h>

IONAPI VOID *MmMapIoSpace(UINT_PTR PhysicalAddress, SIZE_T NumberOfBytes, MM_CACHING_TYPE CacheType) {
    SIZE_T Offset = PhysicalAddress & (PAGE_SIZE - 1);
    if (NumberOfBytes == 0 || NumberOfBytes > (SIZE_T)-1 - Offset - (PAGE_SIZE - 1) ||
        (CacheType != MmCacheWriteBack && CacheType != MmCacheUncached)) {
        return NULL;
    }

    SIZE_T NumPages = MmPageAlignUp(NumberOfBytes + Offset) / PAGE_SIZE;
    UINT_PTR PhysicalBase = MmPageAlignDown(PhysicalAddress);
    if (PhysicalBase > (UINT_PTR)-1 - (NumPages - 1) * PAGE_SIZE) return NULL;

    VOID *VirtualBase = MmAllocateVirtualPagesArena(
        MmKernelArena, NULL, NumPages, MM_ALLOC_UNBACKED, MM_PROT_READ | MM_PROT_WRITE
    );

    if (VirtualBase == NULL) return NULL;

    for (SIZE_T Page = 0; Page < NumPages; Page++) {
        OS_STATUS Status = MmuMapPageEx(
            MmKernelAddressSpace,
            (UINT_PTR)VirtualBase + Page * PAGE_SIZE,
            PhysicalBase + Page * PAGE_SIZE,
            MM_PROT_READ | MM_PROT_WRITE,
            CacheType
        );

        if (!OS_SUCCESS(Status)) {
            KeBugCheckWithStatus(Status, "IO space reservation already has a mapping");
        }
    }

    return (VOID*)((UINT_PTR)VirtualBase + Offset);
}

IONAPI VOID MmUnmapIoSpace(VOID *BaseAddress, SIZE_T NumberOfBytes) {
    SIZE_T Offset = (UINT_PTR)BaseAddress & (PAGE_SIZE - 1);
    ASSERT(NumberOfBytes > 0 && NumberOfBytes <= (SIZE_T)-1 - Offset - (PAGE_SIZE - 1));

    VOID *VirtualBase = (VOID*)MmPageAlignDown((UINT_PTR)BaseAddress);
    SIZE_T NumPages = MmPageAlignUp(NumberOfBytes + Offset) / PAGE_SIZE;
    OS_STATUS Status = MmFreeVirtualPagesArena(MmKernelArena, VirtualBase, NumPages);
    ASSERT(OS_SUCCESS(Status));
}
