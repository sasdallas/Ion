/**
 * @file include/ion/ke/os.h
 * @brief Public-facing OS API
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_OS_H
#define _ION_KE_OS_H

#include <ion/base.h>
#include <ion/ke/ob.h>
#include <ion/ke/ke_types.h>

IONAPI OS_STATUS OsCreateEvent(HANDLE *EventHandle, OBJECT_ATTRIBUTES *ObjectAttributes, KEVENT_TYPE EventType, BOOLEAN InitialState);
IONAPI OS_STATUS OsSetEvent(IN HANDLE EventHandle, OUT OPTIONAL LONG *PreviousState);
IONAPI OS_STATUS OsClearEvent(IN HANDLE EventHandle);
IONAPI OS_STATUS OsResetEvent(IN HANDLE EventHandle, OUT OPTIONAL LONG *PreviousState);

IONAPI OS_STATUS OsCreateDirectoryObject(OUT HANDLE *DirectoryHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes);

#endif
