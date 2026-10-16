/**
 * @file include/ion/ke/lpb.h
 * @brief Loader parameter block
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_LPB_H
#define _ION_KE_LPB_H 1

#include <ion/base.h>

typedef enum _LOADER_MEMORY_TYPE {
    LOADER_MEMORY_AVAIL,
    LOADER_MEMORY_RESERVED,
    LOADER_MEMORY_ACPI_NVS,
    LOADER_MEMORY_ACPI_RECLAIM,
    LOADER_MEMORY_BAD,
    LOADER_MEMORY_KERNEL, // includes modules like HAL     
} LOADER_MEMORY_TYPE;

typedef struct _LOADER_MEMORY_REGION {
    UINT_PTR Base;
    UINT_PTR Length;
    LOADER_MEMORY_TYPE Type;
} LOADER_MEMORY_REGION;

typedef struct _LOADER_MODULE {
    UINT_PTR Base;
    CHAR *Path;
    SIZE_T Size;
    CHAR *Cmdline;
} LOADER_MODULE;

typedef struct _LOADER_FRAMEBUFFER {
    UINT_PTR Address;
    SIZE_T Size;
    UINT64 Width;
    UINT64 Height;
    UINT64 Pitch;
    USHORT Bpp;
    UCHAR MemoryModel;
    UCHAR RedMaskSize;
    UCHAR RedMaskShift;
    UCHAR GreenMaskSize;
    UCHAR GreenMaskShift;
    UCHAR BlueMaskSize;
    UCHAR BlueMaskShift;
} LOADER_FRAMEBUFFER;

typedef struct _LOADER_PARAMETER_BLOCK {
    LOADER_MEMORY_REGION *MemoryRegions;
    SIZE_T NrMemoryRegions;
    UINT_PTR HhdmRegion;

    LOADER_MODULE *LoadedModules;
    SIZE_T NrLoadedModules;

    LOADER_FRAMEBUFFER *Framebuffers;
    SIZE_T NrFramebuffers;
} LOADER_PARAMETER_BLOCK;

#endif
