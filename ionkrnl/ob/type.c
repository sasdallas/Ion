/**
 * @file ionkrnl/ob/type.c
 * @brief Ob type manager
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ob.h>
#include <ion/ke/mm.h>
#include <ion/ke/ke.h>
#include <ion/ion.h>

/* Primary type list */
LIST_ENTRY ObpTypeList;

/* Object type... type */
static OBJECT_TYPE ObpObjectTypeType = {
    .Info = {
        .TargetHeap = NULL,
        .OpenProcedure = NULL,
        .CloseProcedure = NULL,
        .DestroyProcedure = NULL,
        .DumpProcedure = NULL,
        .ParseProcedure = NULL,
        .InvalidAttributes = 0,
    },
    .Name = "ObjectType",
};

OS_STATUS ObInitializeTypeManager() {
    InitializeListHead(&ObpTypeList);
    InitializeListHead(&ObpObjectTypeType.TypeList);
    ObpObjectTypeType.Info.TargetHeap = MmNonpagedHeap;
    return STATUS_SUCCESS;
}

IONAPI OS_STATUS ObCreateObjectType(
    const CHAR *TypeName,
    OBJECT_TYPE_ATTRIBUTES *TypeAttributes,
    OBJECT_TYPE **TypeOutput
)
{
    // XXX: This abuses a semantic of the object manager where it only attempts to insert
    //      objects into the namespace on ObInsertObject, but always allocates ObjectName if present.
    //      ObpPrepareObjectTypesDirectory() will fixup types created before ObObjectTypesDirectory
    //      was able to initialize by setting RootDirectory.
    OBJECT_ATTRIBUTES Attributes = {
        .ObjectName = TypeName,
        .RootDirectory = ObObjectTypesDirectory,
        .Flags = OB_FLAG_PERMANENT_OBJECT,
    };

    // Create the object type
    OBJECT_TYPE *Type;
    OS_STATUS Status = ObCreateObject(&ObpObjectTypeType, &Attributes, sizeof(OBJECT_TYPE), (VOID**)&Type);
    if (!OS_SUCCESS(Status)) {
        return Status;
    }

    // Setup type information
    Type->Name = TypeName;
    RtlCopyMemory(&Type->Info, TypeAttributes, sizeof(OBJECT_TYPE_ATTRIBUTES));

    // Insert into object type directory if available.
    // Object types created before this will be inserted through some hacky code in
    // ObpPrepareObjectTypesDirectory(). The Header already had its TypeName allocated for it
    if (ObpObjectTypesDirectoryObject != NULL) {
        OBJECT_HEADER *Header = OBJECT_GET_HEADER(Type);
        Header->Directory = ObObjectTypesDirectory;
        Status = ObpLinkObject(ObpObjectTypesDirectoryObject, Type, Type->Name, RtlStringLength(Type->Name));
        if (!OS_SUCCESS(Status)) {
            ObDereferenceObject(Type);
            return Status;
        }
    }

    // Type is finished
    InsertTailList(&ObpTypeList, &Type->TypeList);

    if (TypeOutput) {
        *TypeOutput = Type;
    }


    return STATUS_SUCCESS;
}
