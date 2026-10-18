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
#include <stdbool.h>

#ifndef __int64
#define __int64 __INT64_TYPE__
#endif

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned __int64 UINT64;

typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed __int64 INT64;

typedef __int64 LONG64;
typedef unsigned __int64 ULONG64; 

typedef char CHAR;
typedef unsigned char UCHAR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int INT;
typedef unsigned int UINT;
typedef signed long LONG;
typedef unsigned long ULONG;

typedef __int64 LONGLONG;
typedef unsigned __int64 ULONGLONG;


#ifdef _ION64
typedef __int64 INT_PTR;
typedef __int64 LONG_PTR;
typedef unsigned __int64 UINT_PTR;
typedef unsigned __int64 ULONG_PTR;
#else
typedef int INT_PTR;
typedef long LONG_PTR;
typedef unsigned int UINT_PTR;
typedef unsigned long ULONG_PTR;
#endif

typedef unsigned char BYTE;

typedef int BOOL;
typedef BYTE BOOLEAN;

#define TRUE true
#define FALSE false

typedef size_t SIZE_T;
typedef void VOID;

typedef VOID *HANDLE;

#endif
