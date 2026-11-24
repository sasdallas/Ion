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

typedef struct _PATH_VIEW {
    CHAR *Buffer;
    SIZE_T Length;
} PATH_VIEW;

#define ObpAdvancePathView(Pv) do { (Pv)->Buffer++; (Pv)->Length--; } while (0);

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
    OBJECT_DIRECTORY *Directory = ObpRootDirectoryObject;
    VOID *LastObject = Directory;
    OS_STATUS Status = STATUS_SUCCESS;
    BOOLEAN LastObjectReferenced = FALSE;

    // This is an initialization case, ObpRootDirectoryObject
    if (UNLIKELY(ObpRootDirectoryObject == NULL)) {
        if (RootHandle) return STATUS_INVALID_HANDLE;
        ASSERT(RtlStringEquals(ObjectString, "/"));
        ObpRootDirectoryObject = InsertObject;
        ObReferenceObject(InsertObject);
        if (FoundObject) *FoundObject = InsertObject;
        return STATUS_SUCCESS; 
    }

    // A root handle applies to relative names. Absolute names start at /.
    if (RootHandle && ObjectString[0] != OB_PATH_SEPARATOR) {
        Status = ObReferenceObjectByHandle(RootHandle, ObDirectoryObjectType, KernelMode, &LastObject);
        if (!OS_SUCCESS(Status)) return Status;
        LastObjectReferenced = TRUE;
        Directory = LastObject;
    }

    // Tokenize!
    PATH_VIEW View = { .Buffer = ObjectString, .Length = RtlStringLength(ObjectString) };
    while (View.Length) {
        // Skip the first token if it's a /
        if (*View.Buffer == OB_PATH_SEPARATOR) {
            ObpAdvancePathView(&View);
        }

        // Create component path
        PATH_VIEW Component = View;
        while (View.Length && *View.Buffer != OB_PATH_SEPARATOR) {
            ObpAdvancePathView(&View);
        }

        Component.Length -= View.Length;
        if (!Component.Length) continue;

        VOID *NextObject = NULL;

        // If the object wants to parse directories in their own way, allow them to
        // via ParseProcedure.
        OBJECT_HEADER *LastObjectHeader = OBJECT_GET_HEADER(LastObject);
        if (LastObjectHeader->Type->Info.ParseProcedure) {
            ASSERT(0 && "ParseProcedure is not implemented");
        } else {
            if (LastObjectHeader->Type != ObDirectoryObjectType) {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
                goto Cleanup;
            }

            // Parse the directory entry
            Directory = LastObject;
            NextObject = ObpLookupEntryDirectory(Directory, Component.Buffer, Component.Length);

            PATH_VIEW Remaining = View;
            while (Remaining.Length && *Remaining.Buffer == OB_PATH_SEPARATOR) {
                ObpAdvancePathView(&Remaining);
            }

            // Handle the insertion case.
            if (NextObject == NULL) {
                // Didn't get an object, but maybe they wanted to insert it.
                if (InsertObject && !Remaining.Length) {
                    Status = ObpLinkObject(Directory, InsertObject, Component.Buffer, Component.Length);
                    if (!OS_SUCCESS(Status)) {
                        goto Cleanup;
                    }

                    // Upon a success, we need to reference it again to keep it here.
                    ObReferenceObject(InsertObject);
                    NextObject = InsertObject;

                    // We need to also move Component into it
                    CHAR *ComponentName = MmAllocateHeap(MmPagedHeap, Component.Length+1);
                    if (ComponentName == NULL) {
                        Status = STATUS_NO_MEMORY;
                        goto Cleanup;
                    }

                    RtlCopyMemory(ComponentName, Component.Buffer, Component.Length);
                    ComponentName[Component.Length] = 0;

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

        if (LastObjectReferenced) {
            ObDereferenceObject(LastObject);
        }
        
        LastObject = NextObject;
        LastObjectReferenced = TRUE;
    }

    // Check ObjectType now
    if (ObjectType && OBJECT_GET_HEADER(LastObject)->Type != ObjectType) {
        Status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Cleanup;
    }

    if (FoundObject) {
        // If the user wanted to get the root directory, it skipped the reference
        // path, so do it here I guess.
        if (!LastObjectReferenced) {
            ObReferenceObject(LastObject);
        }

        *FoundObject = LastObject;
        return STATUS_SUCCESS;
    }

Cleanup:
    if (LastObjectReferenced) ObDereferenceObject(LastObject);
    return Status;
}
