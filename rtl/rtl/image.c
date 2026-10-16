/**
 * @file rtl/rtl/image.c
 * @brief RTL misc. PE helpers
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

IMAGE_NT_HEADERS* RtlImageNtHeaders(VOID* ImageBase) {
    IMAGE_DOS_HEADER *DosHeader = (IMAGE_DOS_HEADER*)ImageBase;

    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return NULL;
    }

    IMAGE_NT_HEADERS *NtHeaders = (IMAGE_NT_HEADERS*)((UINT_PTR)ImageBase + DosHeader->e_lfanew);   
    if (NtHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return NULL;
    }

    return NtHeaders;
}

VOID* RtlImageDirectoryEntryToData(VOID* BaseAddress, BOOLEAN MappedAsImage, USHORT Directory, SIZE_T *Size) {
    IMAGE_NT_HEADERS *NtHeaders = RtlImageNtHeaders(BaseAddress);

    // impl from ReactOS: https://github.com/mirror/reactos/blob/master/reactos/lib/rtl/image.c#L332
    ULONG Va = NtHeaders->OptionalHeader.DataDirectory[Directory].VirtualAddress;
    *Size = NtHeaders->OptionalHeader.DataDirectory[Directory].Size;

    if (MappedAsImage || Va < NtHeaders->OptionalHeader.SizeOfHeaders) {
        return (VOID*)((ULONG_PTR)BaseAddress + Va);
    }

    ASSERT(0 && "RtlImageRvaToVa not implemented");
}
