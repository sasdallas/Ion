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
};

static const CHAR* RtlStatusStringsError[] = {
    "",
    "An internal error occurred while performing the operation.",
    "An invalid parameter was passed to a system function.",
    "Not enough memory is available to complete the operation.",
    "The executable image is invalid.",
    "A DLL or executable image was not found.",
    "An entrypoint within a DLL/application was not found.",
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
