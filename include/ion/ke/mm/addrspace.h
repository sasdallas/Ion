/**
 * @file include/ion/ke/mm/addrspace.h
 * @brief Address space and page table types
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_MM_ADDRSPACE_H
#define _ION_KE_MM_ADDRSPACE_H 1

#include <ion/base.h>
#include <ion/ke/ke/lock.h>

typedef UINT_PTR MM_PAGETBL;

typedef struct _MM_ADDRESS_SPACE {
    MM_PAGETBL *PageTable;
    KSPIN_LOCK Lock;
} MM_ADDRESS_SPACE;

extern IONAPI MM_ADDRESS_SPACE *MmKernelAddressSpace;

#endif
