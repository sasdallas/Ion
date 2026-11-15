/**
 * @file ionkrnl/ps/process.c
 * @brief Process manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>
#include <ion/ke/ps.h>
#include <ion/ion.h>

/* Last PID in use */
static LONG PspLastPid = 0;

/* Process list */
KSPIN_LOCK PspProcessListLock;
LIST_ENTRY PspProcessList;

VOID PsInitializeProcess(PROCESS *Process, CHAR *ImageName, KPRIORITY Priority) {
    Process->Priority = Priority;
    Process->Pid = InterlockedIncrement(&PspLastPid);
    Process->AddressSpace = NULL;
    
    Process->ThreadCount = 0;
    InitializeListHead(&Process->ThreadList);

    KeInitializeSpinLock(&Process->Spinlock);
    RtlCopyStringN(Process->ImageName, ImageName, sizeof(Process->ImageName));
}

OS_STATUS PsInitializeProcessManager() {
    KeInitializeSpinLock(&PspProcessListLock);
    InitializeListHead(&PspProcessList);
    return STATUS_SUCCESS;
}
