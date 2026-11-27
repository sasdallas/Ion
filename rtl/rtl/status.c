/**
 * @file rtl/rtl/status.c
 * @brief Status to string mapping
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/rtl.h>

static const CHAR* RtlStatusStringsSuccess[] = {
    "The operation completed successfully.",
    "The object name already exists.",
    "The operation is pending.",
};

static const CHAR* RtlStatusStringsError[] = {
    "",
    "An internal error occurred while performing the operation.",
    "An invalid parameter was passed to a system function.",
    "Not enough memory is available to complete the operation.",
    "The executable image is invalid.",
    "A DLL or executable image was not found.",
    "An entrypoint within a DLL/application was not found.",
    "An entry or object was not able to be located.",
    "An operation timed out.",
    "An invalid HANDLE was specified.",
    "Too many files are opened.",
    "The object type given does not match the object type expected.",
    "The object name given is already in use.",
    "An invalid device request code was provided.",
    "Operation not implemented."
};


const CHAR* RtlStatusToString(OS_STATUS Status) {
    if (OS_INFORMATION(Status)) {
        return "Unknown status code";
    } else if (OS_WARNING(Status)) {
        return "Unknown status code";
    } else if (OS_ERROR(Status)) {
        Status &= ~(3ULL << 30);
        if (Status < 0 || Status >= ARRAYLEN(RtlStatusStringsError)) {
            return "Unknown status code";
        }

        return RtlStatusStringsError[Status];
    } else {
        if (Status < 0 || Status >= ARRAYLEN(RtlStatusStringsSuccess)) {
            return "Unknown status code";
        }

        return RtlStatusStringsSuccess[Status];
    }
}
