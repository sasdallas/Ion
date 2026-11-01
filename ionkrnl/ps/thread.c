/**
 * @file ionkrnl/ps/thread.c
 * @brief Thread manager
 *
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 *
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ps.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>

#define PS_KERNEL_STACK_PAGES 8

/* Next thread ID */
static THREAD_ID PspNextThreadId = 1;

extern VOID PspSwitchContext(THREAD_CONTEXT *Old, THREAD_CONTEXT *New);
extern VOID PspInitializeThreadSchedule(THREAD *Thread);

static VOID *PspAllocateKernelStack() {
    // TODO pull these from some other location
    return MmAllocateHeap(MmNonpagedHeap, 24576);
}

IONAPI OS_STATUS PsCreateThread(
    PROCESS *Process,
    VOID (*StartRoutine)(VOID *Context),
    VOID *Context,
    THREAD **ThreadOutput
) {
    OBJECT_ATTRIBUTES ThreadAttributes;
    ObInitializeObjectAttributes(&ThreadAttributes, NULL, 0, NULL);

    THREAD *Thread;
    OS_STATUS Status = ObCreateObject(
        PsThreadObjectType,
        &ThreadAttributes,
        sizeof(THREAD),
        (VOID**)&Thread
    );

    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    KeInitializeSpinLock(&Thread->Spinlock);
    Thread->ThreadId = (THREAD_ID)InterlockedExchangeAdd(&PspNextThreadId, 1);
    Thread->ThreadState = PsThreadInitialized;
    Thread->KernelStack = PspAllocateKernelStack();
    ASSERT(Thread->KernelStack != NULL);

    // Scheduler online
    PspInitializeThreadSchedule(Thread);

    // Insert into process list
    KIRQL OldIrql;
    KeAcquireSpinLock(&Process->Spinlock, &OldIrql);
    Thread->Process = Process;
    InsertTailList(&Process->ThreadList, &Thread->ProcEntry);
    ObReferenceObject(Thread);
    KeReleaseSpinLock(&Process->Spinlock, OldIrql);

    *ThreadOutput = Thread;
    return STATUS_SUCCESS;
}

IONAPI PROCESS *PsGetCurrentProcess() {
    THREAD *Thread = KeGetCurrentThread();
    return Thread != NULL ? Thread->Process : NULL;
}

IONAPI NO_RETURN VOID PsExitThread() {
    KeBugCheckWithMessage("PsExitThread NYI");
}
