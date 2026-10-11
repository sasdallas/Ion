/**
 * @file include/ion/ke/mm/arena.h
 * @brief Arena system
 * 
 * An arena is effectively one big space of virtual addresses
 * The kernel has one and each userspace program has one
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_MM_ARENA_H
#define _ION_KE_MM_ARENA_H 1

#include <ion/base.h>
#include <ion/ke/mm/pfn.h>
#include <ion/ke/ke_types.h>

typedef struct _MM_ARENA_NODE {
    PFN_NUMBER StartPfn;
    SIZE_T NumPages;

    struct _MM_ARENA_NODE *Left;
    struct _MM_ARENA_NODE *Right;
    struct _MM_ARENA_NODE *Parent;
} MM_ARENA_NODE;

typedef struct _MM_ARENA {
    KSPIN_LOCK SpinLock;
    UINT_PTR Base;
    SIZE_T TotalPages;
    MM_ARENA_NODE *Root;
    MM_ARENA_NODE *Leftmost;
} MM_ARENA;

extern MM_ARENA *MmKernelArena;

#endif
