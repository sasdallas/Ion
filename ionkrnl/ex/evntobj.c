/**
 * @file ionkrnl/ex/evntobj.c
 * @brief Executive event object
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ex.h>
#include <ion/ke/ke.h>
#include <ion/ke/os.h>

/* Event object type */
OBJECT_TYPE *ExEventObjectType = NULL;

IONAPI OS_STATUS OsCreateEvent(HANDLE *EventHandle, OBJECT_ATTRIBUTES *ObjectAttributes, KEVENT_TYPE EventType, BOOLEAN InitialState) {
    KEVENT *Event;
    OS_STATUS Status = ObCreateObject(ExEventObjectType, ObjectAttributes, sizeof(KEVENT), (VOID**)&Event);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    KeInitializeEvent(Event, EventType, InitialState);

    return ObInsertObject((VOID*)Event, EventHandle);
}

IONAPI OS_STATUS OsSetEvent(IN HANDLE EventHandle, OUT OPTIONAL LONG *PreviousState) {
    KEVENT *Event;
    OS_STATUS Status = ObReferenceObjectByHandle(EventHandle, ExEventObjectType, KernelMode, (VOID**)&Event);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    DbgPrint("ObReferenceObjectByHandle: %p\n", Event);

    return STATUS_SUCCESS;
}

IONAPI OS_STATUS OsClearEvent(IN HANDLE EventHandle) {
    assert(0 && "OsClearEvent NYI");
}

IONAPI OS_STATUS OsResetEvent(IN HANDLE EventHandle, OUT OPTIONAL LONG *PreviousState) {
    assert(0 && "OsResetEvent NYI");
}
