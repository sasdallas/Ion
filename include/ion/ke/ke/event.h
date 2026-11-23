/**
 * @file include/ion/ke/ke/event.h
 * @brief KEVENT
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_EVENT_H
#define _ION_KE_KE_EVENT_H 1

#include <ion/base.h>
#include <ion/ke/ps/dispatcher.h>

typedef enum _KEVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent,
} KEVENT_TYPE;

typedef struct _KEVENT {
    DISPATCH_HEADER Header;
} KEVENT;

#endif
