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

extern THREAD *PspSwitchContext(THREAD_CONTEXT *Old, THREAD_CONTEXT *New, THREAD *Previous);
extern VOID PspInitializeThreadSchedule(THREAD *Thread);
extern VOID PspFinishThreadSwitch(THREAD *Previous);

NO_RETURN VOID PsThreadStartup(THREAD *Previous, VOID (*StartRoutine)(VOID *), VOID *Context) {
    PspFinishThreadSwitch(Previous);
    StartRoutine(Context);
    PsExitThread();
}

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
    Thread->Process = Process;
    Thread->ThreadId = (THREAD_ID)InterlockedExchangeAdd(&PspNextThreadId, 1);
    Thread->ThreadState = PsThreadInitialized;
    Thread->KernelStack = PspAllocateKernelStack();
    ASSERT(Thread->KernelStack != NULL);

    // Prepare initial thread context
    PspInitializeThreadContext(Thread, StartRoutine, Context);

    // Scheduler online
    PspInitializeThreadSchedule(Thread);

    // Insert into process list
    KIRQL OldIrql;
    KeAcquireSpinLock(&Process->Spinlock, &OldIrql);
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

// Called with interrupts disabled
VOID PspSwitchThread(THREAD *Next) {
    KCPU *Cpu = KeGetCpu();
    THREAD *Current = Cpu->CurrentThread;
    
    Next->ThreadState = PsThreadRunning;
    Cpu->CurrentThread = Next;

    if (Current != NULL && Current->Process->AddressSpace != Next->Process->AddressSpace) {
        MmuSetAddressSpace(Next->Process->AddressSpace);
    }

    THREAD *Previous = PspSwitchContext(&Current->Context, &Next->Context, Current);
    PspFinishThreadSwitch(Previous);
}
