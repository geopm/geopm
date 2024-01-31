/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <sstream>

#include "MSRPath.hpp"
#include "MSRIO.hpp"
#include "geopm/Exception.hpp"

namespace geopm
{

    MSRPath::MSRPath()
        : MSRPath(MSRIO::M_DRIVER_MSRSAFE)
    {

    }

    MSRPath::MSRPath(int driver_type)
        : m_driver_type(driver_type)
    {

    }

    std::string MSRPath::msr_path(int cpu_idx) const
    {
        std::ostringstream path_ss;
        path_ss << "/dev/cpu/" << cpu_idx;
        switch (m_driver_type) {
            case MSRIO::M_DRIVER_MSRSAFE:
                path_ss << "/msr_safe";
                break;
            case MSRIO::M_DRIVER_MSR:
                path_ss << "/msr";
                break;
            default:
                throw Exception("MSRIOImp::msr_path(): Failed to open any of the options for reading msr values",
                                GEOPM_ERROR_MSR_OPEN, __FILE__, __LINE__);
                break;
        }
        return path_ss.str();
    }

    std::string MSRPath::msr_batch_path(void) const
    {
        return "/dev/cpu/msr_batch";
    }
}

