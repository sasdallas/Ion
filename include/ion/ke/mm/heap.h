/**
 * @file include/ion/ke/mm/heap.h
 * @brief Heap system
 * 
 * Heaps in Ion represent dynamically-sized pools. The kernel provides two globally.
 * Heaps can be fully paged or fully nonpaged.
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_MM_HEAP_H
#define _ION_KE_MM_HEAP_H 1

#include <ion/base.h>
#include <ion/list.h>
#include <ion/ke/ke_types.h>

typedef UINT32 MM_TAG;

#define HEAP_TAG(Str) \
    (((UINT32)(Str)[0] << 24) | \
     ((UINT32)(Str)[1] << 16) | \
     ((UINT32)(Str)[2] <<  8) | \
     ((UINT32)(Str)[3]))

typedef enum _MM_HEAP_TYPE {
    MM_HEAP_PAGED,
    MM_HEAP_NONPAGED
} MM_HEAP_TYPE;

typedef struct _MM_HEAP {
    const CHAR* HeapName;
    MM_TAG DefaultTag; // will be passed to MmAllocatePoolEx
    MM_HEAP_TYPE Type;
} MM_HEAP;

extern MM_HEAP* MmPagedHeap;
extern MM_HEAP* MmNonpagedHeap;

#endif
