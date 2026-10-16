/**
 * @file include/ion/ke/ldr.h
 * @brief Executable/DLL loader
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_LDR_H
#define _ION_KE_LDR_H 1

#include <ion/base.h>
#include <ion/list.h>

typedef struct _LOADED_DLL {
    const CHAR* Name;
    UINT_PTR ImageBase;
    SIZE_T ImageSize;

    LIST_ENTRY Entry;
} LOADED_DLL;

VOID *LdrGetSymbol(LOADED_DLL *Dll, const CHAR* SymbolName);
LOADED_DLL *LdrFindDllByName(const CHAR* DllName);
OS_STATUS LdrLoadDll(UINT_PTR ImageBase, SIZE_T ImageSize, LOADED_DLL **DllOut);
OS_STATUS LdrInitialize();
OS_STATUS LdrInitializeHal();

#endif
