/**
 * @file include/ion/rtl/image.h
 * @brief RTL functions for interacting with PE images
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_RTL_IMAGE_H
#define _ION_RTL_IMAGE_H 1

#include <ion/base.h>
#include <ion/pe.h>

IMAGE_NT_HEADERS* RtlImageNtHeaders(VOID* ImageBase);
VOID* RtlImageDirectoryEntryToData(VOID* BaseAddress, BOOLEAN MappedAsImage, USHORT Directory, SIZE_T *Size);

#endif
