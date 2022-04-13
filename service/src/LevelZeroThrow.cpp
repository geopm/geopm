/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "geopm/Exception.hpp"
#include "LevelZero.hpp"

namespace geopm
{
    const LevelZero &levelzero()
    {
        throw Exception("LevelZeroThrow::" + std::string(__func__) +
                        ": GEOPM configured without Level Zero library support.  Please configure with --enable-levelzero",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
}
