/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "MSR.hpp"
#include "Exception.hpp"

namespace geopm
{
    const std::map<std::string, MSR::m_function_e> MSR::M_FUNCTION_STRING = {
        {"scale", M_FUNCTION_SCALE},
        {"log_half", M_FUNCTION_LOG_HALF},
        {"7_bit_float", M_FUNCTION_7_BIT_FLOAT},
        {"overflow", M_FUNCTION_OVERFLOW}
    };

    const std::map<std::string, MSR::m_units_e> MSR::M_UNITS_STRING = {
        {"none", M_UNITS_NONE},
        {"seconds", M_UNITS_SECONDS},
        {"hertz", M_UNITS_HERTZ},
        {"watts", M_UNITS_WATTS},
        {"joules", M_UNITS_JOULES},
        {"celsius", M_UNITS_CELSIUS}
    };

    MSR::m_function_e MSR::string_to_function(const std::string &str)
    {
        auto it = M_FUNCTION_STRING.find(str);
        if (it == M_FUNCTION_STRING.end()) {
            throw Exception("MSR::string_to_units(): invalid function string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    MSR::m_units_e MSR::string_to_units(const std::string &str)
    {
        auto it = M_UNITS_STRING.find(str);
        if (it == M_UNITS_STRING.end()) {
            throw Exception("MSR::string_to_units(): invalid units string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }
}
