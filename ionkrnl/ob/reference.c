/**
 * @file ionkrnl/ob/reference.c
 * @brief ObReference family
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ob.h>
#include <ion/ke/ke.h>
#include <ion/ke/mm.h>
#include <ion/ion.h>

IONAPI VOID ObDereferenceObject(VOID *Body) {
    OBJECT_HEADER *Header = OBJECT_GET_HEADER(Body);
    LONG Count = InterlockedDecrement(&Header->PointerCount);

    if (Count == 0) {
        ASSERT(Header->HandleCount == 0);
        if (Header->Type->Info.DestroyProcedure) {
            Header->Type->Info.DestroyProcedure(Body);
        }

        OS_STATUS Status = MmFreeHeap(Header);
        ASSERT(OS_SUCCESS(Status));
    }
}

IONAPI VOID ObReferenceObject(VOID *Body) {
    OBJECT_HEADER *Header = OBJECT_GET_HEADER(Body);
    InterlockedIncrement(&Header->PointerCount);
}
