/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MSR_HPP_INCLUDE
#define MSR_HPP_INCLUDE

#include <string>
#include <map>

namespace geopm
{
    /// @brief This class encodes how to access fields within an MSR,
    /// but does not hold the state of any registers.
    class MSR
    {
        public:
            enum m_function_e {
                M_FUNCTION_SCALE,           // Only apply scalar value (applied by all functions)
                M_FUNCTION_LOG_HALF,        // 2.0 ^ -X
                M_FUNCTION_7_BIT_FLOAT,     // 2 ^ Y * (1.0 + Z / 4.0) : Y in [0:5), Z in [5:7)
                M_FUNCTION_OVERFLOW,        // Counter that may overflow
            };

            /// @brief Convert a string to the corresponding m_function_e value
            static m_function_e string_to_function(const std::string &str);
        private:
            static const std::map<std::string, m_function_e> M_FUNCTION_STRING;
    };
}

#endif
