/**
 * @file include/ion/ke/ke.h
 * @brief Ion core kernel subsystems (Ke)
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_KE_H
#define _ION_KE_KE_H 1

#include <ion/base.h>

#include <ion/ke/ke_types.h>
#include <ion/ke/lpb.h>

#ifdef __x86_64__
#include <ion/ke/x86_64/ke.h>
#else
#error unknown arch
#endif

extern LOADER_PARAMETER_BLOCK *KeLoaderParameterBlock;
IONAPI LOADER_PARAMETER_BLOCK *KeGetLoaderParameterBlock();

/* Spinlock family */
IONAPI VOID KeInitializeSpinLock(KSPIN_LOCK *SpinLock);
IONAPI VOID KeAcquireSpinLock(KSPIN_LOCK *SpinLock, KIRQL *OldIrql);
IONAPI VOID KeReleaseSpinLock(KSPIN_LOCK *SpinLock, KIRQL NewIrql);
IONAPI VOID KeAcquireSpinLockAtDpcLevel(KSPIN_LOCK *SpinLock);
IONAPI VOID KeReleaseSpinLockAtDpcLevel(KSPIN_LOCK *SpinLock);
IONAPI BOOLEAN KeTryAcquireSpinLockAtDpcLevel(KSPIN_LOCK *SpinLock);

/* Initialize */
NO_RETURN VOID KeInitialize();
VOID KeInitializeBsp();

/* Bugcheck */
IONAPI NO_RETURN VOID KeBugCheckWithMessage(const CHAR* Message, ...);
IONAPI NO_RETURN VOID KeBugCheckTrapFrame(TRAP_FRAME *TrapFrame, const CHAR* Message, ...);
IONAPI NO_RETURN VOID KeBugCheckWithStatus(OS_STATUS Status, const CHAR* Message, ...);

/* Semaphore family */
IONAPI VOID KeInitializeSemaphore(KSEMAPHORE *Semaphore, LONG Count, LONG Limit);
IONAPI LONG KeReleaseSemaphore(KSEMAPHORE *Semaphore, KPRIORITY Boost, LONG Adjustment, BOOLEAN Wait);
IONAPI BOOLEAN KeTryAcquireSemaphore(KSEMAPHORE *Semaphore);

/* IRQL */
IONAPI VOID KeRaiseIrql(KIRQL NewIrql, KIRQL *OldIrql);
IONAPI VOID KeLowerIrql(KIRQL NewIrql);

/* Software interrupt */
IONAPI VOID KeRequestSoftwareInterrupt(KIRQL Irql);
VOID KiAcknowledgeSoftwareInterrupt(KIRQL Irql);
VOID KiDispatchPendingSoftwareInterrupt(KIRQL NewIrql);

/* DPC */
VOID KiInitializeDpcQueue();
VOID KiDpcInterrupt();
IONAPI VOID KeInitializeDpc(KDPC *Dpc, DEFERRED_FUNCTION Function);
IONAPI VOID KeEnqueueDpc(KDPC *Dpc, VOID *Context);

/* CPU */
IONAPI OS_STATUS KeSetInterruptHandler(UINT Vector, VOID (*Handler)(TRAP_FRAME*));

/* Wait */
INT KeProcessObjectWaiters(VOID *Object, BOOLEAN WakeAll, OS_STATUS Status);
OS_STATUS KeWaitForMultipleObjects(
    ULONG Count,
    VOID **Objects,
    KWAIT_REASON WaitReason,
    KPROCESSOR_MODE WaitMode,
    BOOLEAN Alertable,
    INT Timeout,
    KWAIT_BLOCK *WaitBlockArray
);

OS_STATUS KeWaitForSingleObject(
    VOID *Object,
    KWAIT_REASON WaitReason,
    KPROCESSOR_MODE WaitMode,
    BOOLEAN Alertable,
    INT Timeout
);

/* Timer */
VOID KeInitializeTimerSystem();
IONAPI VOID KeInitializeTimerEx(KTIMER *Timer, KTIMER_TYPE Type);
#define KeInitializeTimer(Timer) KeInitializeTimerEx((Timer), NotificationTimer)
VOID KeUpdateTimers();
IONAPI BOOLEAN KeSetTimerEx(KTIMER *Timer, LONGLONG DueTime, LONG Period, KDPC *Dpc);
IONAPI BOOLEAN KeCancelTimer(KTIMER *Timer);

/* Pushlock */
IONAPI VOID KeInitializePushLock(KPUSH_LOCK *PushLock);
IONAPI VOID KeAcquirePushLockExclusiveSlow(KPUSH_LOCK *PushLock);

FORCE_INLINE VOID KeAcquirePushLockExclusive(KPUSH_LOCK *PushLock) {
    if ((InterlockedOrPointer(PushLock, PUSH_LOCK_LOCKED) & PUSH_LOCK_LOCKED) == 0) {
        return;
    }

    return KeAcquirePushLockExclusiveSlow(PushLock);
}

IONAPI VOID KeAcquirePushLockShared(KPUSH_LOCK *PushLock);
IONAPI VOID KeReleasePushLockShared(KPUSH_LOCK *PushLock);
IONAPI VOID KeReleasePushLockExclusive(KPUSH_LOCK *PushLock);

/* Event */
IONAPI VOID KeInitializeEvent(KEVENT *Event, KEVENT_TYPE Type, BOOLEAN State);
IONAPI VOID KeSetEvent(KEVENT *Event, KPRIORITY Increment);
IONAPI VOID KeClearEvent(KEVENT *Event);
IONAPI LONG KeResetEvent(KEVENT *Event);

FORCE_INLINE LONG KeReadStateEvent(KEVENT *Event) {
    return Event->Header.SignalCount;
}

/* Fast mutex */
FORCE_INLINE VOID KeInitializeFastMutex(KFAST_MUTEX *Mutex) { KeInitializePushLock((KPUSH_LOCK*)Mutex); }
FORCE_INLINE VOID KeAcquireFastMutex(KFAST_MUTEX *Mutex) { KeAcquirePushLockExclusive((KPUSH_LOCK*)Mutex); }
FORCE_INLINE VOID KeReleaseFastMutex(KFAST_MUTEX *Mutex) { KeReleasePushLockExclusive((KPUSH_LOCK*)Mutex); }

// KeOnBsp only safe if scheduling is disabled!!!
// otherwise you will get switched off. does unsafe KeGetCpu()->LogicalId read
FORCE_INLINE BOOLEAN KeOnBsp() {
    return KeGetCpu()->LogicalId == 0;
}

#endif
