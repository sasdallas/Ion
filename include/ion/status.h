/**
 * @file include/ion/status.h
 * @brief OS_STATUS
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_STATUS_H
#define _ION_STATUS_H 1

typedef int OS_STATUS;

#define OS_SUCCESS(x) ((x) == STATUS_SUCCESS)

#define __SUCCESS(x) ((unsigned int)(x) | (0ULL << 31))
#define __INFORMATIONAL(x) ((unsigned int)(x) | (0ULL << 31))
#define __WARNING(x) ((unsigned int)(x) | (0ULL << 31))
#define __ERROR(x) ((unsigned int)(x) | (0ULL << 31))

#define STATUS_SUCCESS              __SUCCESS(0x0)
#define STATUS_INTERNAL_ERROR       __ERROR(0x1)
#define STATUS_INVALID_PARAMETER    __ERROR(0x2)
#define STATUS_NO_MEMORY            __ERROR(0x3)

#endif
