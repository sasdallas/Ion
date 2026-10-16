/**
 * @file ionkrnl/mm/heap.c
 * @brief Heap manager
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
#include <ion/ion.h>

/* Sizes of each allocation */
#define HEAP_CACHES 8
static SIZE_T MmHeapCacheSizes[HEAP_CACHES] = {
    16, 32, 64, 128, 256, 512, 1024, 2048
};

/* Caches */
static MM_POOL* MmPagedHeapPools[HEAP_CACHES] = { 0 };
static MM_POOL* MmNonpagedHeapPools[HEAP_CACHES] = { 0 };

/* Magics */
#define HEAP_MAGIC_ALLOCATED        0xCAFEBABE
#define HEAP_MAGIC_FREE             0xC0FFEE11

struct __HEAP_HEADER {
    UINT32 A;  MM_HEAP *B; MM_TAG C; SIZE_T D; SIZE_T E;
};

typedef struct _HEAP_HEADER {
    UINT32 Magic;
    MM_HEAP *Heap;
    MM_TAG Tag;
    SIZE_T CacheSize;
    SIZE_T AllocSize;
    UINT8 Padding[sizeof(struct __HEAP_HEADER) % 16];
} HEAP_HEADER;

/* macro to get heap header */
#define HEAP_GET_HEADER(x) (HEAP_HEADER*)((UINT_PTR)(x) - sizeof(HEAP_HEADER))

/* Heaps */
MM_HEAP *MmNonpagedHeap = NULL;
MM_HEAP *MmPagedHeap = NULL;

/* Pool o' heaps */
MM_POOL *MmHeapPool = NULL;

/* Heap debug */
#define HEAP_DEBUG 1
#ifdef HEAP_DEBUG
#define DBG(...) DbgPrint(__VA_ARGS__)
#define HEAP_ASSERT assert
#else
#define DBG(...) ((void)0)
#define HEAP_ASSERT(x) ((void)0)
#endif

OS_STATUS MmCreateHeap(const CHAR *HeapName, MM_HEAP_TYPE Type, MM_TAG Tag, MM_HEAP **Output) {
    MM_HEAP *Heap = MmAllocatePool(MmHeapPool);
    if (Heap == NULL) {
        return STATUS_NO_MEMORY;
    }

    Heap->HeapName = HeapName;
    Heap->Type = Type;
    Heap->DefaultTag = Tag;
    
    *Output = Heap;
    return STATUS_SUCCESS;
}

OS_STATUS MmDestroyHeap(MM_HEAP *Heap) {
    KeBugCheckWithMessage("MmDestroyHeap NYI");
}

OS_STATUS MmAllocateHeap2(MM_HEAP *Heap, SIZE_T Size, UINT AllocFlags, MM_TAG Tag, VOID **Output) {
    if (Heap == NULL || Output == NULL || Size == 0 ||
        Size > (SIZE_T)-1 - sizeof(HEAP_HEADER)) {
        return STATUS_INVALID_PARAMETER;
    }

    SIZE_T TotalSize = sizeof(HEAP_HEADER) + Size;
    VOID *Allocation = NULL;
    SIZE_T CacheSize = 0;

    if (TotalSize <= MmHeapCacheSizes[HEAP_CACHES - 1]) {
        UINT i = 0;
        while (MmHeapCacheSizes[i] < TotalSize) {
            i++;
        }

        MM_POOL *Pool = Heap->Type == MM_HEAP_PAGED ? MmPagedHeapPools[i] : MmNonpagedHeapPools[i];
        OS_STATUS Status = MmAllocatePool2(Pool, POOL_FLAG_UNINIT, &Allocation);
        if (!OS_SUCCESS(Status)) {
            return Status;
        }

        CacheSize = MmHeapCacheSizes[i];
    } else {
        Allocation = MmAllocateVirtualPagesArena(
            MmKernelArena,
            NULL,
            MmPageAlignUp(TotalSize) / PAGE_SIZE,
            (AllocFlags & POOL_FLAG_UNINIT) ? MM_ALLOC_UNINIT : 0,
            MM_PROT_READ | MM_PROT_WRITE
        );

        if (Allocation == NULL) {
            return STATUS_NO_MEMORY;
        }
    }

    HEAP_HEADER *Header = Allocation;
    Header->Magic = HEAP_MAGIC_ALLOCATED;
    Header->Heap = Heap;
    Header->Tag = Tag;
    Header->CacheSize = CacheSize;
    Header->AllocSize = Size;

    VOID *Data = (UINT8 *)Allocation + sizeof(HEAP_HEADER);

    if (CacheSize != 0 && (AllocFlags & HEAP_FLAG_UNINIT) == 0) {
        RtlZeroMemory(Data, Size);
    }

    *Output = Data;

    return STATUS_SUCCESS;
}

OS_STATUS MmFreeHeap(VOID *Allocation) {
    if (Allocation == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    HEAP_HEADER *Header = HEAP_GET_HEADER(Allocation);
    if (Header->Magic != HEAP_MAGIC_ALLOCATED || Header->Heap == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    MM_HEAP *Heap = Header->Heap;
    SIZE_T CacheSize = Header->CacheSize;
    SIZE_T AllocSize = Header->AllocSize;

    if (CacheSize != 0) {
        UINT i = 0;
        while (i < HEAP_CACHES && MmHeapCacheSizes[i] != CacheSize) {
            i++;
        }
        if (i == HEAP_CACHES) {
            return STATUS_INVALID_PARAMETER;
        }

        MM_POOL *Pool = Heap->Type == MM_HEAP_PAGED
            ? MmPagedHeapPools[i] : MmNonpagedHeapPools[i];
        Header->Magic = HEAP_MAGIC_FREE;
        return MmFreePool(Pool, Header);
    }

    if (AllocSize > (SIZE_T)-1 - sizeof(HEAP_HEADER) - (PAGE_SIZE - 1)) {
        return STATUS_INVALID_PARAMETER;
    }

    SIZE_T NumPages = MmPageAlignUp(sizeof(HEAP_HEADER) + AllocSize) / PAGE_SIZE;
    Header->Magic = HEAP_MAGIC_FREE;
    return MmFreeVirtualPagesArena(MmKernelArena, Header, NumPages);
}

OS_STATUS MiInitializeKernelHeaps() {
    OS_STATUS Status = MmCreatePool("HeapObjPool", MM_POOL_NONPAGED, sizeof(MM_HEAP), POOL_ALIGN_NONE, &MmHeapPool);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    for (UINT i = 0; i < HEAP_CACHES; i++) {
        Status = MmCreatePool("NonpagedHeapPool", MM_POOL_NONPAGED, MmHeapCacheSizes[i], POOL_ALIGN_NONE, &MmNonpagedHeapPools[i]);
        if (!OS_SUCCESS(Status)) {
            return Status;
        }

        Status = MmCreatePool("PagedHeapPool", MM_POOL_PAGED, MmHeapCacheSizes[i], POOL_ALIGN_NONE, &MmPagedHeapPools[i]);
        if (!OS_SUCCESS(Status)) {
            return Status;
        }
    }

    Status = MmCreateHeap("DflPagedHeap", MM_HEAP_PAGED, HEAP_TAG("DflP"), &MmPagedHeap);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    Status = MmCreateHeap("DflNonpagedHeap", MM_HEAP_NONPAGED, HEAP_TAG("DflN"), &MmNonpagedHeap);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}
