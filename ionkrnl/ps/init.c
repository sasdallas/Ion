/**
 * @file ionkrnl/ps/init.c
 * @brief Process initialization code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ps.h>
#include <ion/ke/ob.h>
#include <ion/ion.h>

typedef struct _SYSTEM_PROCESS {
    OBJECT_HEADER ObjectHeader;
    PROCESS Process;
} SYSTEM_PROCESS;

/* Public object types */
OBJECT_TYPE *PsThreadObjectType = NULL;
OBJECT_TYPE *PsProcessObjectType = NULL;

OS_STATUS PsInitialize() {
    return STATUS_SUCCESS;
}
