/**
 * @file rtl/rtl/memory.c
 * @brief Memory-management (RtlZeroMemory and friends)
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

VOID RtlFillMemory(VOID* Fill, UCHAR Value, SIZE_T Size) {
    UCHAR* Memory = (UCHAR*)Fill;
    while (Size--) {
        *Memory++ = Value;
    }
}

VOID RtlCopyMemory(VOID* Destination, const VOID* Source, SIZE_T Size) {
    UCHAR *Dest = (UCHAR*)Destination;
    UCHAR *Src = (UCHAR*)Source;

    while (Size--) {
        *Dest++ = *Src++;
    }
}

INT RtlCompareString(CHAR* String1, CHAR* String2, SIZE_T Size) {
    while (Size && *String1 && (*String1 == *String2)) {
        String1++;
        String2++;
        Size--;
    }

    if (Size == 0) {
        return 0;
    }

    return *String1 - *String2;
}

BOOLEAN RtlStringEquals(CHAR* String1, CHAR* String2) {
    while (*String1 && *String2 && *String1 == *String2) {
        String1++;
        String2++;
    }

    if (*String1 == *String2) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void *memset(void *s, int ch, size_t n) {
    unsigned char *p = (unsigned char*)s;
    for (unsigned i = 0; i < n; i++) p[i] = ch;
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    for (unsigned i = 0; i < n; i++) *d++ = *s++;
    return dest;
}
