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

/* uACPI needs these, GCC might also generate calls to them, its fine keeping them here */

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

int memcmp(const void *s1, const void *s2, size_t n) {
    if (s1 == s2) return 0;
    unsigned char *p1 = (unsigned char *)s1;
    unsigned char *p2 = (unsigned char *)s2;
    size_t r = n;
    while (r) {
        if (*p1 != *p2) return (*p1 - *p2);
        p1++; p2++; r--;
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    char *d = (char*)dest;
    const char *s = (const char*)src;
    if (d == s || !n) return dest;


    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }

    return dest;
}
