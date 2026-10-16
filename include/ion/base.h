/**
 * @file include/ion/base.h
 * @brief Ion basic declarations
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_BASE_H
#define _ION_BASE_H 1

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

#include <ion/types.h>
#include <ion/status.h>

// Parameter specifications, purely verbose.
#define IN
#define OUT
#define INOUT
#define OPTIONAL

#ifndef __INTELLISENSE__
// The kernel provides the HAL stubcalls, whereas the HAL provides the vtable
#ifdef KERNEL
#define IONHALAPI __declspec(dllexport)
#else
#define IONHALAPI __declspec(dllimport)
#endif

#ifdef KERNEL
#define IONAPI __declspec(dllexport)
#else
#define IONAPI __declspec(dllimport)
#endif
#else
#define IONAPI
#define IONHALAPI
#endif

// Helper macros
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define CONTAINING_RECORD(Pointer, Type, Field) ((Type*)((uintptr_t)(Pointer) - (uintptr_t)offsetof(Type, Field)))
#define BIT(x) (1ULL << (x))
#define ALIGN_UP(val, align) (((val) + (align) - 1) & ~((align)-1))
#define ALIGN_DOWN(val, align) (((val) & ~((align)-1)))
#define IS_ALIGNED(val, align) (((val) & ((align)-1))==0)
#define ALIGNOF(type) offsetof(struct { char c; type member; }, member)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define FIELD_OFFSET(structure, member) offsetof((structure), (member))
#define ARRAYLEN(array) (sizeof(array) / sizeof(array[0]))

// Declarations
#define PACKED __attribute__((packed))
#define NO_RETURN __attribute__((noreturn))
#define UNUSED __attribute__((unused))
#define ALWAYS_INLINE __attribute__((always_inline))
#define FORCE_INLINE ALWAYS_INLINE static inline

#endif
