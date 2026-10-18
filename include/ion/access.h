/**
 * @file include/ion/access.h
 * @brief ACCESS_MASK
 * 
 * @ref Ideas from from NT:
 *      https://learn.microsoft.com/en-us/windows/win32/secauthz/access-mask
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_ACCESS_H
#define _ION_ACCESS_H 1

#include <ion/types.h>

typedef UINT32 ACCESS_MASK;

// bits 0-15 are available for custom usage
#define GENERIC_READ            0x80000000UL
#define GENERIC_WRITE           0x40000000UL
#define GENERIC_EXECUTE         0x20000000UL
#define GENERIC_ALL             0x10000000UL

#endif
