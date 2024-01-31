/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <string>

#include "geopm/Exception.hpp"

namespace geopm
{
    class NVMLDevicePool;

    NVMLDevicePool &nvml_device_pool(const int num_cpu)
    {
        throw Exception("NVMLDevicePoolThrow::" + std::string(__func__) +
                        ": GEOPM configured without nvml library support.  Please configure with --enable-nvml",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

}
