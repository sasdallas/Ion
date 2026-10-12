/**
 * @file include/ion/ke/mm/pool.h
 * @brief pool system
 * 
 * A pool in Ion is effectively similar to a kmem cache in Linux, whereas it allocates statically-sized
 * objects and hands them out. Drivers and the kernel are free to create pools as they please.
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_MM_POOL_H
#define _ION_KE_MM_POOL_H 1

#include <ion/base.h>
#include <ion/list.h>
#include <ion/ke/ke_types.h>

typedef struct _MM_POOL_SLAB {
    LIST_ENTRY Entry;

    VOID **FreeList;
    SIZE_T FreeCount;
} MM_POOL_SLAB;

typedef enum _MM_POOL_TYPE {
    MM_POOL_PAGED,
    MM_POOL_NONPAGED
} MM_POOL_TYPE;

typedef struct _MM_POOL {
    const CHAR* Name;
    KSPIN_LOCK Lock;
    MM_POOL_TYPE Type;

    LIST_ENTRY Free;
    LIST_ENTRY Partial;
    LIST_ENTRY Full;
    
    SIZE_T FreeSlabCount;

    SIZE_T ObjectSize;
    SIZE_T ObjectsPerSlab;
    SIZE_T ObjectAlignment;
    SIZE_T ObjectOffset;
    SIZE_T ObjectRealSize;
    SIZE_T SlabSize;
} MM_POOL;

#define POOL_FREE_THRESHOLD 5
#define POOL_ALIGN_NONE ((SIZE_T)0)

#endif
