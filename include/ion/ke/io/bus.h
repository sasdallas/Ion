/**
 * @file include/ion/ke/io/bus.h
 * @brief I/O bus manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_IO_BUS_H
#define _ION_KE_IO_BUS_H 1

#include <ion/base.h>
#include <ion/list.h>
#include <ion/ke/ke.h>

struct _DRIVER_OBJECT;
struct _BUS_OBJECT;

// A bus type is a global container for all bus instances.
// For example, if one has 2 USB controllers, they both go under the global
// BUS_TYPE bus.
typedef struct _BUS_TYPE {
    const CHAR *TypeName;
    KPUSH_LOCK Lock;

    struct _BUS_OBJECT *ParentBus;
    struct _DRIVER_OBJECT *Driver;

    LIST_ENTRY Instances;
    SIZE_T NrInstances;

    // IsPending is used here because some device drivers may load before others.
    // This doesn't happen a lot but its useful to have the flexibility
    BOOLEAN IsPending;
    HANDLE DirectoryHandle;
} BUS_TYPE;

typedef struct _BUS_OBJECT {
    CHAR *InstanceName;
    KPUSH_LOCK Lock;
    BUS_TYPE *Type;

    LIST_ENTRY Entry;

    LIST_ENTRY Devices;
    SIZE_T NrDevices;

    HANDLE DirectoryHandle;
} BUS_OBJECT;

extern BUS_OBJECT *IoRootBus;

#endif
