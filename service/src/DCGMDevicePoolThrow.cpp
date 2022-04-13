/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <string>

#include "geopm/Exception.hpp"

namespace geopm
{
    class DCGMDevicePool;

    const DCGMDevicePool &dcgm_device_pool()
    {
        throw Exception("DCGMDevicePoolThrow::" + std::string(__func__) +
                        ": GEOPM configured without dcgm library support.  Please configure with --enable-dcgm",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

}
