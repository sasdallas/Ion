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

typedef LONG OS_STATUS;

#define OS_SUCCESS(x) ((OS_STATUS)(x) >= 0)
#define OS_INFORMATION(x) ((((ULONG)(x)) >> 30) == 1)
#define OS_WARNING(x) ((((ULONG)(x)) >> 30) == 2)
#define OS_ERROR(x) ((((ULONG)(x)) >> 30) == 3)

#define __SUCCESS(x) ((ULONG)(x) | (0ULL << 30))
#define __INFORMATIONAL(x) ((ULONG)(x) | (1ULL << 30))
#define __WARNING(x) ((ULONG)(x) | (2ULL << 30))
#define __ERROR(x) ((ULONG)(x) | (3ULL << 30))

#define STATUS_SUCCESS                  __SUCCESS(0x0)
#define STATUS_INTERNAL_ERROR           __ERROR(0x1)
#define STATUS_INVALID_PARAMETER        __ERROR(0x2)
#define STATUS_NO_MEMORY                __ERROR(0x3)
#define STATUS_INVALID_IMAGE            __ERROR(0x4)
#define STATUS_DLL_NOT_FOUND            __ERROR(0x5)
#define STATUS_ENTRYPOINT_NOT_FOUND     __ERROR(0x6)

#define STATUS_FORMAT   "0x%08x"

#endif
