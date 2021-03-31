/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
