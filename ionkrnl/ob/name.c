/**
 * @file ionkrnl/ob/name.c
 * @brief Object manager name system
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

/**
 * Look up an object from a directory.
 * This is the core of the Object Manager's namespace system. It does the path
 * tokenization and necessary lookups to find (and possibly insert) an object at a directory.
 */
OS_STATUS ObpLookupObjectName(
    HANDLE RootHandle,
    CHAR *ObjectString,
    ULONG Attributes,
    OBJECT_TYPE *ObjectType,
    VOID *ParseContext,
    VOID *InsertObject,
    VOID **FoundObject
)
{
    if (FoundObject) *FoundObject = NULL;
    if (!ObjectString) return STATUS_INVALID_PARAMETER;

    // Begin searching at the root directory.

    // This is an initialization case, ObpRootDirectoryObject
    if (UNLIKELY(ObpRootDirectoryObject == NULL)) {
        if (RootHandle) return STATUS_INVALID_HANDLE;
        ASSERT(RtlStringEquals(ObjectString, "/"));
        ObpRootDirectoryObject = InsertObject;
        ObReferenceObject(InsertObject);
        if (FoundObject) *FoundObject = InsertObject;
        return STATUS_SUCCESS; 
    }

    OBJECT_DIRECTORY *Directory = ObpRootDirectoryObject;
    VOID *LastObject = Directory;
    BOOLEAN WasReferenced = FALSE;
    BOOLEAN DidDoTheParse = FALSE;

    // A root handle applies to relative names, and absolute names start at /.
    if (RootHandle && ObjectString[0] != OB_PATH_SEPARATOR) {
        OS_STATUS Status = ObReferenceObjectByHandle(RootHandle, ObDirectoryObjectType, KernelMode, &LastObject);
        if (!OS_SUCCESS(Status)) {
            return Status;
        }

        WasReferenced = TRUE;
        Directory = LastObject;
    }

    // Tokenize!
    OB_STRING_VIEW View = { .Buffer = ObjectString, .Length = RtlStringLength(ObjectString) };
    OB_STRING_VIEW FullView = View; // Create a clone for objects with a parse method
    OS_STATUS Status = STATUS_SUCCESS;
    while (View.Length) {
        // Skip the first token if it's a /
        if (*View.Buffer == OB_PATH_SEPARATOR) {
            ObpAdvanceStringView(&View);
        }

        // Create component path
        OB_STRING_VIEW Component = View;
        while (View.Length && *View.Buffer != OB_PATH_SEPARATOR) {
            ObpAdvanceStringView(&View);
        }

        Component.Length -= View.Length;
        if (!Component.Length) continue;

        VOID *NextObject = NULL;

        // If the object wants to parse directories in their own way, allow them to
        // via ParseProcedure.
        OBJECT_HEADER *LastObjectHeader = OBJECT_GET_HEADER(LastObject);
        if (LastObjectHeader->Type->Info.ParseProcedure) {
            Status = LastObjectHeader->Type->Info.ParseProcedure(
                LastObject,
                ObjectType,
                &FullView,
                &View,
                ParseContext,
                &NextObject
            );

            if (!OS_SUCCESS(Status)) {
                goto Cleanup;
            }

            if (!NextObject) {
                Status = STATUS_NOT_FOUND;
                goto Cleanup;
            }

            if (WasReferenced) ObDereferenceObject(LastObject);
            LastObject = NextObject;
            WasReferenced = TRUE;
            DidDoTheParse = TRUE;
            break;
        } else {
            if (LastObjectHeader->Type != ObDirectoryObjectType) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
                goto Cleanup;
            }

            // Parse the directory entry
            Directory = LastObject;
            NextObject = ObpLookupEntryDirectory(Directory, Component.Buffer, Component.Length);

            OB_STRING_VIEW Remaining = View;
            while (Remaining.Length && *Remaining.Buffer == OB_PATH_SEPARATOR) {
                ObpAdvanceStringView(&Remaining);
            }

            // Handle the insertion case.
            if (NextObject == NULL) {
                // Didn't get an object, but maybe they wanted to insert it.
                if (InsertObject && !Remaining.Length) {
                    CHAR *ComponentName = MmAllocateHeap(MmPagedHeap, Component.Length + 1);
                    if (ComponentName == NULL) {
                        Status = STATUS_NO_MEMORY;
                        goto Cleanup;
                    }

                    RtlCopyMemory(ComponentName, Component.Buffer, Component.Length);
                    ComponentName[Component.Length] = 0;

                    Status = ObpLinkObject(Directory, InsertObject, Component.Buffer, Component.Length);
                    if (!OS_SUCCESS(Status)) {
                        MmFreeHeap(ComponentName);
                        goto Cleanup;
                    }

                    // The directory owns a reference to the inserted object.
                    ObReferenceObject(InsertObject);

                    // The lookup result also needs its own reference.
                    ObReferenceObject(InsertObject);

                    NextObject = InsertObject;

                    // Replace the names
                    OBJECT_HEADER *Header = OBJECT_GET_HEADER(InsertObject);
                    MmFreeHeap(Header->ObjectName);
                    Header->ObjectName = ComponentName;
                } else {
                    Status = STATUS_NOT_FOUND;
                }
            }
        }

        if (!OS_SUCCESS(Status)) {
            goto Cleanup;
        }

        if (WasReferenced) {
            ObDereferenceObject(LastObject);
        }
        
        LastObject = NextObject;
        WasReferenced = TRUE;
    }

    // We also have to parse the last object, e.g. /Devices/Beep still needs the Parse method called on it
    OBJECT_HEADER *LastObjectHeader = OBJECT_GET_HEADER(LastObject);
    if (!InsertObject && !DidDoTheParse && LastObjectHeader->Type->Info.ParseProcedure) {
        // Validate object type here since we have to do two validations
        if (ObjectType && LastObjectHeader->Type != ObjectType) {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Cleanup;
        }

        VOID *ParsedObject = NULL;
        Status = LastObjectHeader->Type->Info.ParseProcedure(
            LastObject,
            ObjectType,
            &FullView,
            &View,
            ParseContext,
            &ParsedObject
        );

        if (!OS_SUCCESS(Status)) {
            goto Cleanup;
        }

        if (ParsedObject == NULL) {
            Status = STATUS_NOT_FOUND;
            goto Cleanup;
        }

        if (WasReferenced) {
            ObDereferenceObject(LastObject);
        }

        LastObject = ParsedObject;
        WasReferenced = TRUE;
    }

    // Check ObjectType now
    if (ObjectType && OBJECT_GET_HEADER(LastObject)->Type != ObjectType) {
        Status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Cleanup;
    }

    if (FoundObject) {
        // If the user wanted to get the root directory, it skipped the reference
        // path, so do it here I guess.
        if (!WasReferenced) {
            ObReferenceObject(LastObject);
        }

        *FoundObject = LastObject;
        return STATUS_SUCCESS;
    }

Cleanup:
    if (WasReferenced) ObDereferenceObject(LastObject);
    return Status;
}
