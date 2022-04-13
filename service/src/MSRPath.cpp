/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <sstream>

#include "MSRPath.hpp"
#include "geopm/Exception.hpp"

namespace geopm
{
    std::string MSRPath::msr_path(int cpu_idx,
                                  int fallback_idx)
    {
        std::ostringstream path_ss;
        path_ss << "/dev/cpu/" << cpu_idx;
        switch (fallback_idx) {
            case M_FALLBACK_MSRSAFE:
                path_ss << "/msr_safe";
                break;
            case M_FALLBACK_MSR:
                path_ss << "/msr";
                break;
            default:
                throw Exception("MSRIOImp::msr_path(): Failed to open any of the options for reading msr values",
                                GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);
                break;
        }
        return path_ss.str();
    }

    std::string MSRPath::msr_batch_path(void)
    {
        return "/dev/cpu/msr_batch";
    }
}

