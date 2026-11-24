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
extern NO_RETURN VOID PspSwitchContext2(THREAD_CONTEXT *New);
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
    Thread->ThreadId = (THREAD_ID)InterlockedExchangeAdd(&PspNextThreadId, 1);
    Thread->ThreadState = PsThreadInitialized;
    Thread->KernelStack = PspAllocateKernelStack();
    ASSERT(Thread->KernelStack != NULL);

    ObReferenceObject(Process);
    Thread->Process = Process;

    // Prepare initial thread context
    PspInitializeThreadContext(Thread, StartRoutine, Context);

    // Scheduler online
    PspInitializeThreadSchedule(Thread);

    // Insert into process list
    KIRQL OldIrql;
    KeAcquireSpinLock(&Process->Spinlock, &OldIrql);
    InsertTailList(&Process->ThreadList, &Thread->ProcEntry);
    Process->ThreadCount++;
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
    THREAD *Current = KeGetCurrentThread();
    ASSERT(Current != NULL && Current != KeGetCpu()->IdleThread);
    ASSERT(KeGetCpu()->Irql == PASSIVE_LEVEL);

    PsLockThread(Current);
    Current->ThreadState = PsThreadTerminated;
    PsYieldThreadLocked();
    KeBugCheckWithMessage("Terminated thread was rescheduled?!");
}

VOID PspReapThread(THREAD *Thread) {
    // We're currently on another thread's context, whatever was switched to post-PsExitThread
    // TODO:    Maybe fixup this reap process to happen in a DPC instead of invading whatever
    //          thread we switched to.

    // Free kernel stack
    VOID *Stack = Thread->KernelStack;
    Thread->KernelStack = NULL;
    OS_STATUS Status = MmFreeHeap(Stack);
    ASSERT(OS_SUCCESS(Status));

    // Decrease thread count
    PROCESS *Process = Thread->Process;
    KeAcquireSpinLockAtDpcLevel(&Process->Spinlock);
    RemoveEntryList(&Thread->ProcEntry);
    Process->ThreadCount--;
    KeReleaseSpinLockAtDpcLevel(&Process->Spinlock);

    ObDereferenceObject(Thread); // Process list reference
}

// DestroyProcedure for the thread object type
VOID PspDestroyThreadObject(VOID *Object) {
    THREAD *Thread = Object;
    if (Thread->Process != NULL) {
        ObDereferenceObject(Thread->Process);
    }
}

// Called with interrupts disabled
VOID PspSwitchThread(THREAD *Next) {
    KCPU *Cpu = KeGetCpu();
    THREAD *Current = Cpu->CurrentThread;
    
    Next->ThreadState = PsThreadRunning;
    Cpu->CurrentThread = Next;

    MmuSetAddressSpace(Next->Process->AddressSpace);

    THREAD *Previous = PspSwitchContext(&Current->Context, &Next->Context, Current);
    PspFinishThreadSwitch(Previous);
}

// Called with interrupts disabled
// Variant used to orphan the current thread
NO_RETURN VOID PspSwitchThread2(THREAD *Next) {
    KCPU *Cpu = KeGetCpu();
    Cpu->CurrentThread = Next;
    Next->ThreadState = PsThreadRunning;
    MmuSetAddressSpace(Next->Process->AddressSpace);
    PspSwitchContext2(&Next->Context);
}
