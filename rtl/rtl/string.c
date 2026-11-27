/**
 * @file rtl/rtl/string.c
 * @brief String functions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

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

SIZE_T RtlStringLength(CHAR *String) {
    SIZE_T Length = 0;
    while (*String++) {
        Length += 1;
    }
    return Length;
}

size_t strlen(char *string) {
    SIZE_T Length = 0;
    while (*string++) {
        Length += 1;
    }
    return Length;
}

VOID RtlCopyStringN(CHAR* Destination, const CHAR* Source, SIZE_T MaxSize) {
    while (MaxSize--) {
        if (*Source) {
            *Destination++ = *Source++;
        } else {
            *Destination = 0;
        }
    }
}
