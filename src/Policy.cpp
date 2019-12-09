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

#include "Policy.hpp"

#include <cmath>

#include <sstream>

#include "Exception.hpp"
#include "Helper.hpp"

namespace geopm
{
    Policy::Policy()
    {

    }

    Policy::Policy(const std::vector<double> &values)
        : m_values(values)
    {

    }

    Policy::Policy(std::initializer_list<double> list)
        : m_values(list)
    {

    }

    size_t Policy::size(void) const
    {
        return m_values.size();
    }

    double &Policy::operator[](size_t index)
    {
        return m_values[index];
    }

    bool Policy::operator==(const Policy &other) const
    {
        bool equal = true;
        auto it = m_values.begin();
        auto ito = other.m_values.begin();
        for (; equal && it != m_values.end() && ito != other.m_values.end() ;
             ++it, ++ito) {
            if (!(std::isnan(*it) && std::isnan(*ito)) &&
                *it != *ito) {
                equal = false;
            }
        }
        // check trailing values
        for (; equal && it != m_values.end(); ++it) {
            if (!std::isnan(*it)) {
                equal = false;
            }
        }
        for (; equal && ito != other.m_values.end(); ++ito) {
            if (!std::isnan(*ito)) {
                equal = false;
            }
        }
        return equal;
    }

    bool Policy::operator!=(const Policy &other) const
    {
        return !(*this == other);
    }

    std::string Policy::to_string(const std::string &delimiter) const
    {
        std::ostringstream temp;
        for (int ii = 0; ii < m_values.size(); ++ii) {
            if (ii != 0) {
                temp << delimiter;
            }
            if (std::isnan(m_values.at(ii))) {
                temp << "NAN";
            }
            else {
                temp << geopm::string_format_double(m_values.at(ii));
            }
        }
        return temp.str();
    }

    /// @todo: replace similar implementation in geopm_agent_policy_json
    std::string Policy::to_json(const std::vector<std::string> &policy_names) const
    {
        if (policy_names.size() != m_values.size()) {
            throw Exception("Policy::to_json(): incorrect number of policy names.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::stringstream output_str;

        std::string policy_value;
        output_str << "{";
        for (size_t idx = 0; idx < m_values.size(); ++idx) {
            if (idx > 0) {
                output_str << ", ";
            }
            if (std::isnan(m_values.at(idx))) {
                policy_value = "\"NAN\"";
            }
            else {
                policy_value = geopm::string_format_double(m_values.at(idx));
            }
            output_str << "\"" << policy_names.at(idx) << "\": " << policy_value;
        }
        output_str << "}";
        return output_str.str();
    }

    std::vector<double> Policy::to_vector(void) const
    {
        return m_values;
    }

    void Policy::pad_nan_to(size_t size)
    {
        if (size < m_values.size()) {
            throw Exception("Policy::pad_to_nan(): size of policy cannot be reduced.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_values.resize(size, NAN);
    }
}


std::ostream& operator<<(std::ostream &os, const geopm::Policy &policy)
{
    os << policy.to_string(", ");
    return os;
}
