/**
 * @file include/ion/ke/ke/wait.h
 * @brief Waiting definitions
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_WAIT_H
#define _ION_KE_KE_WAIT_H 1

#include <ion/base.h>
#include <ion/ke/ps/dispatcher.h>

// if someone tries to KeWaitForMultipleObjects on more than THREAD_WAIT_BLOCKS,
// they must allocate a wait block array
#define THREAD_WAIT_BLOCKS      8
#define MAX_WAIT_BLOCKS         32

typedef enum _KWAIT_TYPE {
    WaitAll,
    WaitAny,
} KWAIT_TYPE;

typedef enum _KWAIT_REASON {
    Executive,
    UserRequest
} KWAIT_REASON;

struct _THREAD;
typedef struct _KWAIT_BLOCK {
    LIST_ENTRY Entry;
    DISPATCH_HEADER *Header;
    UINT Index;
    struct _THREAD *Thread;
} KWAIT_BLOCK;

#endif
