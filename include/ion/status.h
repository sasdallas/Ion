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

// success status
#define STATUS_SUCCESS                  __SUCCESS(0x0)
#define STATUS_OBJECT_NAME_EXISTS       __SUCCESS(0x1)
#define STATUS_PENDING                  __SUCCESS(0x2)

// error status
#define STATUS_INTERNAL_ERROR           __ERROR(0x1)
#define STATUS_INVALID_PARAMETER        __ERROR(0x2)
#define STATUS_NO_MEMORY                __ERROR(0x3)
#define STATUS_INVALID_IMAGE            __ERROR(0x4)
#define STATUS_DLL_NOT_FOUND            __ERROR(0x5)
#define STATUS_ENTRYPOINT_NOT_FOUND     __ERROR(0x6)
#define STATUS_NOT_FOUND                __ERROR(0x7)
#define STATUS_TIMEOUT                  __ERROR(0x8)
#define STATUS_INVALID_HANDLE           __ERROR(0x9)
#define STATUS_TOO_MANY_OPENED_FILES    __ERROR(0x10)
#define STATUS_OBJECT_TYPE_MISMATCH     __ERROR(0x11)
#define STATUS_OBJECT_NAME_COLLISION    __ERROR(0x12)
#define STATUS_INVALID_DEVICE_REQUEST   __ERROR(0x13)
#define STATUS_NOT_IMPLEMENTED          __ERROR(0x14)

// Wait values that I didn't want to lump with success
#define STATUS_WAIT_0                   __SUCCESS(0x0)
#define STATUS_WAIT_1                   __SUCCESS(0x1)
#define STATUS_WAIT_2                   __SUCCESS(0x2)
#define STATUS_WAIT_3                   __SUCCESS(0x3)
#define STATUS_WAIT_4                   __SUCCESS(0x4)
#define STATUS_WAIT_5                   __SUCCESS(0x5)
#define STATUS_WAIT_6                   __SUCCESS(0x6)
#define STATUS_WAIT_7                   __SUCCESS(0x7)
#define STATUS_WAIT_8                   __SUCCESS(0x8)
#define STATUS_WAIT_9                   __SUCCESS(0x9)
#define STATUS_WAIT_10                  __SUCCESS(0xA)
#define STATUS_WAIT_11                  __SUCCESS(0xB)
#define STATUS_WAIT_12                  __SUCCESS(0xC)
#define STATUS_WAIT_13                  __SUCCESS(0xD)
#define STATUS_WAIT_14                  __SUCCESS(0xE)
#define STATUS_WAIT_15                  __SUCCESS(0xF)
#define STATUS_WAIT_16                  __SUCCESS(0x10)
#define STATUS_WAIT_17                  __SUCCESS(0x11)
#define STATUS_WAIT_18                  __SUCCESS(0x12)
#define STATUS_WAIT_19                  __SUCCESS(0x13)
#define STATUS_WAIT_20                  __SUCCESS(0x14)
#define STATUS_WAIT_21                  __SUCCESS(0x15)
#define STATUS_WAIT_22                  __SUCCESS(0x16)
#define STATUS_WAIT_23                  __SUCCESS(0x17)
#define STATUS_WAIT_24                  __SUCCESS(0x18)
#define STATUS_WAIT_25                  __SUCCESS(0x19)
#define STATUS_WAIT_26                  __SUCCESS(0x1A)
#define STATUS_WAIT_27                  __SUCCESS(0x1B)
#define STATUS_WAIT_28                  __SUCCESS(0x1C)
#define STATUS_WAIT_29                  __SUCCESS(0x1D)
#define STATUS_WAIT_30                  __SUCCESS(0x1E)
#define STATUS_WAIT_31                  __SUCCESS(0x1F)
#define STATUS_WAIT_PLACEHOLDER         __SUCCESS(0x20)

#define STATUS_FORMAT   "0x%08x"

#endif
