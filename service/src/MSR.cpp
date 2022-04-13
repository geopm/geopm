/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "MSR.hpp"
#include "geopm/Exception.hpp"

namespace geopm
{
    const std::map<std::string, MSR::m_function_e> MSR::M_FUNCTION_STRING = {
        {"scale", M_FUNCTION_SCALE},
        {"log_half", M_FUNCTION_LOG_HALF},
        {"7_bit_float", M_FUNCTION_7_BIT_FLOAT},
        {"overflow", M_FUNCTION_OVERFLOW}
    };

    MSR::m_function_e MSR::string_to_function(const std::string &str)
    {
        auto it = M_FUNCTION_STRING.find(str);
        if (it == M_FUNCTION_STRING.end()) {
            throw Exception("MSR::string_to_function(): invalid function string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }
}
