/**
 * @file include/ion/ke/io/file.h
 * @brief File object
 *
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_IO_FILE_H
#define _ION_KE_IO_FILE_H 1

#include <ion/base.h>
#include <ion/list.h>
#include <ion/ke/ke/lock.h>

struct _DEVICE_OBJECT;

// Dispositions for IoCreateFile
#define FILE_CREATE         0
#define FILE_OPEN           1
#define FILE_OPEN_IF        2

typedef struct _FILE_OBJECT {
    struct _DEVICE_OBJECT *DeviceObject;
    ULONG Flags;
    CHAR *FileName;

    KSPIN_LOCK IrpListLock;
    LIST_ENTRY IrpList;
} FILE_OBJECT;

#endif
