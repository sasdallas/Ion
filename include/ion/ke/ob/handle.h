/**
 * @file include/ion/ke/ob/handle.h
 * @brief Object manager handle information
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_OB_HANDLE_H
#define _ION_KE_OB_HANDLE_H 1

#include <ion/base.h>
#include <ion/ke/ke/pushlock.h>

typedef union _HANDLE_TABLE_ENTRY {
    union {
        VOID *Object;
        ULONG ObAttributes;
        ULONGLONG Value;
    };

    union {
        ULONG NextFreeTableEntry;
    };
} HANDLE_TABLE_ENTRY;


typedef struct _HANDLE_TABLE {
    HANDLE_TABLE_ENTRY *Table;
    KPUSH_LOCK HandleTableLock[4];
    KPUSH_LOCK HandleContentionEvent;

    ULONG FirstFreeHandle;
    HANDLE_TABLE_ENTRY *LastFreeHandleEntry;
    LONG HandleCount;
} HANDLE_TABLE;

// in the never ending quest to larp Windows
#define OBJ_HANDLE_TAGBITS          0x03
#define OBJ_INDEX_TO_HANDLE(x) ((x) << 2)
#define OBJ_HANDLE_TO_INDEX(x) (((ULONG_PTR)(x) & ~3) >> 2)

#define ObpEntryGetObject(TableEntry) ((VOID*)((TableEntry)->Value & ~7ULL))
#define ObpEntryLocked(TableEntry) (!!((TableEntry)->Value & 1))

#endif
