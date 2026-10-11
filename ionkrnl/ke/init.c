/**
 * @file ionkrnl/ke/init.c
 * @brief KeInitialize code
 * 
 * 
 * @copyright
 * This file is part of the Ion operating system.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2026 sasdallas
 */

#include <ion/ke/ke.h>

/* Loader block */
LOADER_PARAMETER_BLOCK *KeLoaderParameterBlock = NULL;
