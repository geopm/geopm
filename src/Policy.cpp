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

#include "Policy.hpp"

#include <algorithm>
#include <sstream>

#include "Exception.hpp"
#include "Helper.hpp"

namespace geopm
{
    Policy::Policy(const std::vector<PolicyField> &fields)
    {
        size_t idx = 0;
        m_default_values.resize(fields.size());
        for (const auto &ff : fields) {
            m_name_index[ff.name] = idx;
            m_default_values[idx] = ff.default_value;
            ++idx;
        }
        m_values = m_default_values;

#ifdef GEOPM_DEBUG
        // assert consistent vector sizes
        if (fields.size() != m_values.size() ||
            fields.size() != m_default_values.size()) {
            throw Exception("Policy(): mismatch in internal vector sizes",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
    }

    std::vector<double> Policy::to_vector(void) const
    {
        /// @todo: can't just return m_values because of NAN.
        /// This could be handled differently with dedicated get/set methods
        /// instead of double& accessor, which does not allow us to
        /// capture resets from writing NAN.  However, this method
        /// allows update(vector) to be a fast copy.
        std::vector<double> result(m_values.size());
        for (size_t ii = 0; ii < result.size(); ++ii) {
            // TODO: const function can't call non-const operator[]
            //result[ii] = operator[](ii);
            if (std::isnan(m_values[ii])) {
                result[ii] = m_default_values[ii];
            }
            else {
                result[ii] = m_values[ii];
            }
        }
        return result;
    }

    bool Policy::operator==(const Policy &other) const
    {
        return false;
    }

    bool Policy::operator!=(const Policy &other) const
    {
        return false;
    }

    double& Policy::operator[](const std::string &name)
    {
        auto it = m_name_index.find(name);
        if (it == m_name_index.end()) {
            throw Exception("Policy::operator[]: invalid policy field name",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
#ifdef GEOPM_DEBUG
        // assert consistent indices
        if (it->second < 0 || it->second >= m_values.size()) {
            throw Exception("Policy(): mismatch in internal data structures",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        double &result = m_values[it->second];
        if (std::isnan(result)) {
            result = m_default_values[it->second];
        }
        return result;
    }

    const double& Policy::operator[](const std::string &name) const
    {
        return const_cast<const double&>(operator[](name));
    }

    double& Policy::operator[](size_t index)
    {
        if (index >= m_values.size()) {
            throw Exception("Policy::operator[]: policy field index out of bounds",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        double &result = m_values[index];
        if (std::isnan(result)) {
            result = m_default_values[index];
        }
        return result;
    }

    const double& Policy::operator[](size_t index) const
    {
        return const_cast<const double&>(operator[](index));
    }

    void Policy::update(const std::string &json)
    {

    }

    void Policy::update(const std::vector<std::string> &values)
    {

    }

    std::string Policy::to_json(void) const
    {
        /// @todo: see geopm_agent_policy_json_partial
        /// need to ensure coverage and remove repeated code.
        /// @todo: prints the policy in alpha order; want it to be
        /// in policy order?
        std::ostringstream output_str;
        output_str << "{";
        size_t idx = 0;
        for (const auto &kv : m_name_index) {
            if (idx > 0) {
                output_str << ", ";
            }
            std::string policy_name = kv.first;
            std::string policy_value = geopm::string_format_double(m_values[kv.second]);
            output_str << "\"" << policy_name << "\": " << policy_value;
            ++idx;
        }
        output_str << "}";
        return output_str.str();
    }
}
