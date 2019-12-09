/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef POLICY_HPP_INCLUDE
#define POLICY_HPP_INCLUDE

#include <vector>
#include <string>

namespace geopm
{
    /// @brief The Policy class is used to handle operations on a
    ///        policy as a vector of doubles, such as comparison and
    ///        formatting.
    class Policy
    {
        public:
            Policy();
            Policy(const std::vector<double> &values);
            Policy(std::initializer_list<double> list);
            Policy(const Policy &other) = default;
            virtual ~Policy() = default;

            /// @brief Returns the number of values in the policy.
            size_t size(void) const;

            /// @brief Access an element of the policy by index.
            double &operator[](size_t index);

            /// @brief Equality comparison operator.  Trailing NANs are
            ///        not considered when checking for equality.
            bool operator==(const Policy &other) const;
            bool operator!=(const Policy &other) const;

            /// @brief Format the Policy vector as a character-delimited
            ///        list.
            std::string to_string(const std::string &delimiter) const;

            /// @brief Format the Policy values as a JSON string.
            /// @param [in] policy_names String names to use for keys
            ///        of each value in order.
            std::string to_json(const std::vector<std::string> &policy_names) const;

            /// @brief Convert the policy values to a std::vector
            std::vector<double> to_vector(void) const;

        private:
            double m_temp;
            std::vector<double> m_values;
    };
}

std::ostream& operator<<(std::ostream &os, const geopm::Policy &policy);

#endif
