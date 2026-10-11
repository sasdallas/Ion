/**
 * @file include/ion/types.h
 * @brief Declares the basic types used in Ion
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_TYPES_H
#define _ION_TYPES_H 1

#include <stdint.h>
#include <stddef.h>

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;

typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed long long INT64;

typedef char CHAR;
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef signed long LONG;
typedef unsigned long ULONG;

#ifdef _ION64
typedef __UINTPTR_TYPE__ UINT_PTR;
typedef __UINTPTR_TYPE__ ULONG_PTR;
#else
typedef unsigned int UINT_PTR;
typedef unsigned long ULONG_PTR;
#endif

typedef unsigned char BYTE;

typedef int BOOL;
typedef BYTE BOOLEAN;

typedef size_t SIZE_T;
typedef void VOID;

#endif
