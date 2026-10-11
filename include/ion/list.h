/**
 * @file include/ion/list.h
 * @brief List macros
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_LIST_H
#define _ION_LIST_H 1

#include <ion/base.h>

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY;

FORCE_INLINE VOID InitializeListHead(LIST_ENTRY *ListHead) {
    ListHead->Blink = ListHead;
    ListHead->Flink = ListHead;
}

FORCE_INLINE VOID InsertHeadList(LIST_ENTRY *ListHead, LIST_ENTRY *Entry) {
    LIST_ENTRY *OldFlink = ListHead->Flink;
    ListHead->Flink = Entry;
    Entry->Flink = OldFlink;
    Entry->Blink = ListHead;
    OldFlink->Blink = Entry;
}

FORCE_INLINE VOID InsertTailList(LIST_ENTRY *ListHead, LIST_ENTRY *Entry) {
    LIST_ENTRY *OldBlink = ListHead->Blink;
    ListHead->Blink = Entry;
    Entry->Blink = OldBlink;
    Entry->Flink = ListHead;
    OldBlink->Flink = Entry;
}

FORCE_INLINE LIST_ENTRY *RemoveHeadList(LIST_ENTRY *ListHead) {
    if (ListHead->Flink == ListHead) {
        return ListHead;
    }

    LIST_ENTRY *Flink = ListHead->Flink;
    ListHead->Flink = Flink->Flink;
    ListHead->Flink->Blink = ListHead;
    return Flink;
}

FORCE_INLINE LIST_ENTRY *RemoveTailList(LIST_ENTRY *ListHead) {
    if (ListHead->Blink == ListHead) {
        return ListHead;
    }

    LIST_ENTRY *Blink = ListHead->Blink;
    ListHead->Blink = Blink->Blink;
    ListHead->Blink->Flink = ListHead;
    return Blink;
}

FORCE_INLINE BOOLEAN RemoveEntryList(LIST_ENTRY *Entry) {
    Entry->Blink->Flink = Entry->Flink;
    Entry->Flink->Blink = Entry->Blink;

    if (Entry->Flink == Entry->Blink) {
        return TRUE;
    }

    return FALSE;
}

FORCE_INLINE BOOLEAN IsListEmpty(LIST_ENTRY *ListHead) {
    if (ListHead->Flink == ListHead) {
        return TRUE;
    }

    return FALSE;
}

#endif
