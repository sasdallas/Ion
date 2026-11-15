/**
 * @file include/ion/ke/ke/dpc.h
 * @brief Deferred Procedure Call
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_DPC_H
#define _ION_KE_KE_DPC_H 1

#include <ion/base.h>
#include <ion/list.h>

struct _KDPC;
typedef VOID (*DEFERRED_FUNCTION)(struct _KDPC *Dpc, VOID *Context);

typedef struct _KDPC {
    LIST_ENTRY Entry;
    DEFERRED_FUNCTION Function;
    VOID *Context;
    BOOLEAN IsEnqueued;
} KDPC;

#endif
