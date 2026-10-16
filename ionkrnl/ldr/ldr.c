/**
 * @file ionkrnl/ldr/ldr.c
 * @brief Primary PE file loader
 *
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ldr.h>
#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/ion.h>

/* TODO: Make this a Ke arch-specific call */
extern CHAR __ImageBase;

/* Loaded DLL pool */
static MM_POOL *LdrpDllPool = NULL;

/* Loaded DLL list */
static LIST_ENTRY LdrLoadedDllList;
SIZE_T LdrNrLoadedDlls = 0;

static OS_STATUS LdrAddNewImage(VOID *ImageBase, LOADED_DLL **DllOut) {
    IMAGE_NT_HEADERS *NtHeaders = RtlImageNtHeaders(ImageBase);
    if (NtHeaders == NULL) {
        DbgPrint("LdrAddNewImage detected invalid image\n");
        return STATUS_INVALID_IMAGE;
    }

    SIZE_T ExportSize;
    IMAGE_EXPORT_DIRECTORY *Exports = RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ExportSize);
    if (ExportSize < sizeof(IMAGE_EXPORT_DIRECTORY) || Exports->Name >= NtHeaders->OptionalHeader.SizeOfImage) {
        return STATUS_INVALID_IMAGE;
    }

    LOADED_DLL *Dll = MmAllocatePool(LdrpDllPool);
    if (Dll == NULL) {
        return STATUS_NO_MEMORY;
    }

    Dll->Name = (CHAR*)((UINT_PTR)ImageBase + Exports->Name);
    Dll->ImageBase = (UINT_PTR)ImageBase;
    Dll->ImageSize = NtHeaders->OptionalHeader.SizeOfImage;

    // Add to DLL list
    InsertTailList(&LdrLoadedDllList, &Dll->Entry);
    LdrNrLoadedDlls += 1;

    *DllOut = Dll;

    return STATUS_SUCCESS;
}

LOADED_DLL *LdrFindDllByName(const CHAR* DllName) {
    // TODO Lock + hashmap maybe?

    LIST_ENTRY *Ent = LdrLoadedDllList.Flink;
    while (Ent != &LdrLoadedDllList) {
        LOADED_DLL *Dll = CONTAINING_RECORD(Ent, LOADED_DLL, Entry);

        if (RtlStringEquals(Dll->Name, DllName)) {
            return Dll;
        }

        Ent = Ent->Flink;
    }

    return NULL;
}

VOID *LdrGetSymbol(LOADED_DLL *Dll, const CHAR* SymbolName) {
    SIZE_T DirectorySize;
    IMAGE_EXPORT_DIRECTORY *Exports = RtlImageDirectoryEntryToData(
        (VOID*)Dll->ImageBase,
        TRUE,
        IMAGE_DIRECTORY_ENTRY_EXPORT,
        &DirectorySize
    );

    if (Exports == NULL || DirectorySize < sizeof(IMAGE_EXPORT_DIRECTORY)) {
        return NULL;
    }

    UINT32 *Names = (VOID*)(Dll->ImageBase + Exports->AddressOfNames);
    USHORT *Ordinals = (VOID*)(Dll->ImageBase + Exports->AddressOfNameOrdinals);
    UINT32 *Functions = (VOID*)(Dll->ImageBase + Exports->AddressOfFunctions);

    // TODO: Maybe its worth creating a symbol database?

    // Perform a binary search
    UINT32 Low = 0;
    UINT32 High = Exports->NumberOfNames;

    while (Low < High) {
        UINT32 Middle = Low + (High - Low) / 2;
        
        CHAR *LeftName = (CHAR*)(Dll->ImageBase + Names[Middle]);
        CHAR *RightName = SymbolName;

        while (*LeftName && *RightName && *LeftName == *RightName) {
            LeftName++;
            RightName++;
        }
        
        INT Compare = (UCHAR)*LeftName - (UCHAR)*RightName;
        if (Compare < 0) {
            Low = Middle+1;
        } else if (Compare > 0) {
            High = Middle;
        } else {
            if (Ordinals[Middle] >= Exports->NumberOfFunctions) {
                return NULL;
            }

            UINT32 Rva = Functions[Ordinals[Middle]];
            if (Rva >= Dll->ImageSize) {
                return NULL;
            }

            return (VOID*)(Dll->ImageBase + Rva);
        }
    }

    return NULL;
}

static OS_STATUS LdrRelocateImage(VOID *ImageBase, IMAGE_NT_HEADERS *NtHeaders) {
    UINT_PTR Delta = (UINT_PTR)ImageBase - NtHeaders->OptionalHeader.ImageBase;
    SIZE_T ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
    if (Delta == 0) {
        return STATUS_SUCCESS;
    }

    SIZE_T DirectorySize;
    UINT_PTR Pos = (UINT_PTR)RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_BASERELOC, &DirectorySize);
    if (Pos == 0x0) {
        return STATUS_INVALID_IMAGE;
    }

    UINT_PTR End = Pos + DirectorySize;
    while (Pos < End) {
        UINT32 PageRva = *(UINT32*)Pos;
        UINT32 BlockSize = *(UINT32*)(Pos + 4);

        USHORT *Entries = (USHORT*)(Pos + 8);
        for (UINT32 i = 0; i < (BlockSize - 8) / sizeof(USHORT); i++) {
            UINT32 Type = Entries[i] >> 12;
            if (Type == 0) {
                continue;
            }

            UINT32 Offset = Entries[i] & 0xfff;
            *(UINT64*)((UINT_PTR)ImageBase + PageRva + Offset) += Delta;
        }

        Pos += BlockSize;
    }

    return STATUS_SUCCESS;
}

static OS_STATUS LdrResolveImports(VOID *ImageBase, IMAGE_NT_HEADERS *NtHeaders) {
    SIZE_T ImageSize = NtHeaders->OptionalHeader.SizeOfImage;
    
    // Get ImportDirectory
    SIZE_T DirectorySize;
    IMAGE_IMPORT_DESCRIPTOR *Import = RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &DirectorySize);
    if (Import == NULL) {
        return STATUS_INVALID_IMAGE;
    }

    UINT_PTR End = (UINT_PTR)Import + DirectorySize;
    for (; (UINT_PTR)(Import + 1) <= End && Import->Name; Import++) {
        CHAR *DllName = (CHAR*)((UINT_PTR)ImageBase + Import->Name);
        UINT32 LookupRva = Import->OriginalFirstThunk ? Import->OriginalFirstThunk : Import->FirstThunk;

        LOADED_DLL *Dll = LdrFindDllByName(DllName);
        if (Dll == NULL) {
            DbgPrint("LdrResolveImports failed, missing DLL %s\n", DllName);
            return STATUS_DLL_NOT_FOUND;
        }

        IMAGE_THUNK_DATA *Lookup = (VOID*)((UINT_PTR)ImageBase + LookupRva);
        IMAGE_THUNK_DATA *Thunk = (VOID*)((UINT_PTR)ImageBase + Import->FirstThunk);
        SIZE_T Count = MIN((ImageSize - LookupRva) / sizeof(IMAGE_THUNK_DATA), (ImageSize - Import->FirstThunk) / sizeof(IMAGE_THUNK_DATA));
        BOOLEAN Terminated = FALSE;
        for (SIZE_T i = 0; i < Count; i++) {
            if (Lookup[i].u1.Function == 0) {
                Terminated = TRUE;
                break;
            }

            IMAGE_IMPORT_BY_NAME *ImportName = (VOID*)((UINT_PTR)ImageBase + Lookup[i].u1.Function);
            VOID *Address = LdrGetSymbol(Dll, (CHAR*)ImportName->Name);
            if (Address == NULL) {
                return STATUS_INVALID_IMAGE;
            }

            Thunk[i].u1.Function = (UINT_PTR)Address;
        }

        if (!Terminated) {
            return STATUS_INVALID_IMAGE;
        }
    }

    return STATUS_SUCCESS;
}

OS_STATUS LdrLoadDll(UINT_PTR ImageBase, SIZE_T ImageSize, LOADED_DLL **DllOut) {
    if (ImageSize < sizeof(IMAGE_DOS_HEADER)) {
        return STATUS_INVALID_IMAGE;
    }

    IMAGE_DOS_HEADER *DosHeader = (VOID*)ImageBase;
    IMAGE_NT_HEADERS *NtHeaders = RtlImageNtHeaders((VOID*)ImageBase);
    if (NtHeaders == NULL) {
        return STATUS_INVALID_IMAGE;
    }

    SIZE_T SectionOffset = DosHeader->e_lfanew + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + NtHeaders->FileHeader.SizeOfOptionalHeader;
    SIZE_T MappedSize = NtHeaders->OptionalHeader.SizeOfImage;

    SIZE_T NrPages = MmPageAlignUp(MappedSize) / PAGE_SIZE;

    VOID *Image = MmAllocateVirtualPagesArena(
        MmKernelArena,
        NULL,
        NrPages,
        0,
        MM_PROT_READ | MM_PROT_WRITE | MM_PROT_EXEC
    );

    if (Image == NULL) {
        return STATUS_NO_MEMORY;
    }

    // Relocate the new image
    RtlCopyMemory(Image, (VOID*)ImageBase, NtHeaders->OptionalHeader.SizeOfHeaders);

    IMAGE_SECTION_HEADER *Section = (VOID*)(ImageBase + SectionOffset);
    for (USHORT i = 0; i < NtHeaders->FileHeader.NumberOfSections; i++, Section++) {
        SIZE_T SectionSize = MAX(Section->Misc.VirtualSize, Section->SizeOfRawData);
        RtlCopyMemory(
            (VOID*)((UINT_PTR)Image + Section->VirtualAddress),
            (VOID*)(ImageBase + Section->PointerToRawData),
            Section->SizeOfRawData
        );
    }

    // Relocate NT headers now
    NtHeaders = RtlImageNtHeaders(Image);

    OS_STATUS Status = LdrRelocateImage(Image, NtHeaders);
    if (!OS_SUCCESS(Status)) {
        DbgPrint("LdrRelocateImage failed with status 0x%08x\n", Status);
        MmFreeVirtualPagesArena(MmKernelArena, Image, NrPages);
        return Status;
    }

    Status = LdrResolveImports(Image, NtHeaders);
    if (!OS_SUCCESS(Status)) {
        DbgPrint("LdrResolveImports failed with status 0x%08x\n", Status);
        MmFreeVirtualPagesArena(MmKernelArena, Image, NrPages);
        return Status;
    }


    LOADED_DLL *Dll;
    Status = LdrAddNewImage(Image, &Dll);
    if (!OS_SUCCESS(Status)) {
        MmFreeVirtualPagesArena(MmKernelArena, Image, NrPages);
        return Status;
    }

    *DllOut = Dll;
    return STATUS_SUCCESS;
}

OS_STATUS LdrInitializeHal() {
    // HACK: The HAL should be the first module loaded
    if (KeLoaderParameterBlock->NrLoadedModules < 1) {
        return STATUS_DLL_NOT_FOUND;
    }

    LOADER_MODULE *HalModule = &KeLoaderParameterBlock->LoadedModules[0];
    LOADED_DLL *HalDll;
    OS_STATUS Status = LdrLoadDll(HalModule->Base, HalModule->Size, &HalDll);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // The HAL entrypoint is at HalInitSystem
    typedef OS_STATUS (*HAL_STARTUP)();
    HAL_STARTUP Startup = LdrGetSymbol(HalDll, "HalInitSystem");
    if (Startup == NULL) {
        return STATUS_ENTRYPOINT_NOT_FOUND;
    }

    // Run the entrypoint
    Status = Startup();
    if (!OS_SUCCESS(Status)) {
        DbgPrint("HalInitSystem failed with status 0x%08x\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

OS_STATUS LdrInitialize() {
    OS_STATUS Status = MmCreatePool("LoadedDllPool", MM_POOL_NONPAGED, sizeof(LOADED_DLL), POOL_ALIGN_NONE, &LdrpDllPool);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    InitializeListHead(&LdrLoadedDllList);

    // Register the kernel as a loaded DLL
    LOADED_DLL *KernelDll;
    Status = LdrAddNewImage((VOID*)&__ImageBase, &KernelDll);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Debug
    DbgPrint("LdrInitialize finished, added kernel object (%s)\n", KernelDll->Name);

    return STATUS_SUCCESS;
}
