/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef GEOPM_LIMITS_H_INCLUDE
#define GEOPM_LIMITS_H_INCLUDE

/*
 * String length allocated for GEOPM names (like signal and control
 * names). Has same value as NAME_MAX from limits.h for historical
 * reasons.
 */
static const size_t GEOPM_NAME_MAX = 255UL;
/*
 * String length allocated for GEOPM messages (like C error messages
 * derived from C++ exceptions).  Has same value as PATH_MAX from
 * linux/limits.h for historical reasons.
 */
static const size_t GEOPM_MESSAGE_MAX = 4096ULL;

#endif
