/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_DEBUG_HPP_INCLUDE
#define GEOPM_DEBUG_HPP_INCLUDE

#include "config.h"  // for GEOPM_DEBUG

#include "geopm/Exception.hpp"

#ifdef GEOPM_DEBUG
/// Used to check for errors that should never occur unless there is a
/// mistake in internal logic.  These checks will be removed in
/// release builds.
#define GEOPM_DEBUG_ASSERT(condition, fail_message)                             \
    if (!(condition)) {                                                         \
        throw Exception(std::string(__func__) + ": " + fail_message,            \
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);                 \
    }
#else
#define GEOPM_DEBUG_ASSERT(condition, fail_message)
#endif

#endif
