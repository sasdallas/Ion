/**
 * @file rtl/rtl/assert.c
 * @brief RtlAssertFail
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

#ifdef KERNEL
#include <ion/ke/ke.h>
#endif

NO_RETURN VOID RtlAssertFail(CHAR *Statement, CHAR *File, INT Line, const CHAR *Function) {
#ifdef KERNEL
    KeBugCheckWithMessage("Assertion \"%s\" failed\n      From %s:%d in function %s", Statement, File, Line, Function);
#else
    /* TODO */
#endif

    for (;;);
}
