/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_ACCESS_H_INCLUDE
#define GEOPM_ACCESS_H_INCLUDE

#include <stddef.h>
#include "geopm_public.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Fill result with the msr-safe allowlist file
///        contents reflecting all known MSRs for the
///        specified platform.
/// @param result_max [in] Size of the result buffer
/// @param result [in, out] Buffer to which the msr-safe
///        allowlist is written.
/// @return Zero on success, error code on failure.
int GEOPM_PUBLIC
    geopm_msr_allowlist(size_t result_max, char *result);

#ifdef __cplusplus
}
#endif
#endif
