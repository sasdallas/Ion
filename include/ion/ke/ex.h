/**
 * @file include/ion/ke/ex.h
 * @brief Executive
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#ifndef _ION_KE_EX_H
#define _ION_KE_EX_H 1

#include <ion/base.h>
#include <ion/ke/ob/object.h>

extern OBJECT_TYPE *ExEventObjectType;

NO_RETURN VOID ExInitialize();

#endif
