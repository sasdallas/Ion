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
#include <ion/ke/lpb.h>

typedef struct _LOADED_DLL {
    const CHAR* Name;
    UINT_PTR ImageBase;
    SIZE_T ImageSize;

    LIST_ENTRY Entry;
} LOADED_DLL;

VOID *LdrGetSymbol(LOADED_DLL *Dll, const CHAR* SymbolName);
BOOLEAN LdrFindSymbolByAddress(UINT_PTR Address, const CHAR **DllName, const CHAR **SymbolName, UINT_PTR *Offset);
LOADED_DLL *LdrFindDllByName(const CHAR* DllName);
OS_STATUS LdrLoadDll(UINT_PTR ImageBase, SIZE_T ImageSize, LOADED_DLL **DllOut);
OS_STATUS LdrMapDll(UINT_PTR ImageBase, SIZE_T ImageSize, LOADED_DLL **DllOut);
OS_STATUS LdrResolveDllImports(LOADED_DLL *Dll);
OS_STATUS LdrInitialize();
OS_STATUS LdrInitializeHal();

FORCE_INLINE OS_STATUS LdrLoadBootModule(LOADER_MODULE *Module, LOADED_DLL **DllOut) {
    return LdrLoadDll(Module->Base, Module->Size, DllOut);
}

FORCE_INLINE OS_STATUS LdrMapBootModule(LOADER_MODULE *Module, LOADED_DLL **DllOut) {
    return LdrMapDll(Module->Base, Module->Size, DllOut);
}

OS_STATUS LdrLoadBootDrivers();

#endif
