/**
 * @file rtl/rtl/print.c
 * @brief Print functions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#define STB_SPRINTF_IMPLEMENTATION
#include <ion/third_party/_stb_sprintf.h>
#include <ion/rtl.h>

int RtlFormatStringVa(char *buffer, unsigned int length, const char *format, va_list ap) {
    return stbsp_vsnprintf(buffer, length, format, ap);
}

int RtlFormatString(char *buffer, unsigned int length, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int r = RtlFormatStringVa(buffer, length, format, ap);
    va_end(ap);
    return r;
}
