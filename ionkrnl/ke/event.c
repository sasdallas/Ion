/**
 * @file ionkrnl/ke/event.c
 * @brief Kernel event system
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>


IONAPI VOID KeInitializeEvent(KEVENT *Event, KEVENT_TYPE Type, BOOLEAN State) {
    PsInitializeDispatcher(&Event->Header, Type == NotificationEvent ? SatisfyNone : SatisfyReset);

    if (State) {
        Event->Header.SignalCount = 1;
    }
}

IONAPI VOID KeSetEvent(KEVENT *Event, KPRIORITY Increment) {
    KIRQL Irql;
    PsLockDispatcher(&Event->Header, &Irql);
    Event->Header.SignalCount = 1;
    KeProcessObjectWaiters(Event, (Event->Header.Satisfaction == SatisfyNone), STATUS_SUCCESS);
    PsUnlockDispatcher(&Event->Header, Irql);
}

IONAPI VOID KeClearEvent(KEVENT *Event) {
    KIRQL Irql;
    PsLockDispatcher(&Event->Header, &Irql);
    Event->Header.SignalCount = 0;
    PsUnlockDispatcher(&Event->Header, Irql);
}

IONAPI LONG KeResetEvent(KEVENT *Event) {
    KIRQL Irql;
    PsLockDispatcher(&Event->Header, &Irql);
    LONG LastState = Event->Header.SignalCount;
    Event->Header.SignalCount = 0;
    PsUnlockDispatcher(&Event->Header, Irql);
    return LastState;
}
