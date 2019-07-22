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

#include "DebugIOGroup.hpp"

#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "config.h"

namespace geopm
{
    DebugIOGroup::DebugIOGroup(const PlatformTopo &topo,
                               std::shared_ptr<std::vector<double> > value_cache)
        : m_topo(topo)
        , m_value_cache(value_cache)
        , m_num_reg_signals(0)
    {
        if (m_value_cache == nullptr) {
            throw Exception("DebugIOGroup(): value_cache cannot be null.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void DebugIOGroup::register_signal(const std::string &name, int domain_type)
    {
        if (m_signal_name.find(name) != m_signal_name.end()) {
            throw Exception("DebugIOGroup::register_signal(): signal " + name + " already registered.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_signal_domain[name] = domain_type;
        int num_domain = m_topo.num_domain(domain_type);
        for (int idx = 0; idx < num_domain; ++idx) {
            if (m_num_reg_signals >= m_value_cache->size()) {
                throw Exception("DebugIOGroup::register_signal(): number of registered signals was greater than size of shared vector provided.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_signal_idx[std::make_pair(name, idx)] = m_num_reg_signals;
            ++m_num_reg_signals;
        }
        m_signal_name.insert(name);
    }

    std::set<std::string> DebugIOGroup::signal_names(void) const
    {
        return m_signal_name;
    }

    std::set<std::string> DebugIOGroup::control_names(void) const
    {
        return {};
    }

    bool DebugIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_name.find(signal_name) != m_signal_name.end();
    }

    bool DebugIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int DebugIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = m_signal_domain.at(signal_name);
        }
        return result;
    }

    int DebugIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int DebugIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::push_signal(): signal_name " + signal_name +
                            " not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != m_signal_domain.at(signal_name)) {
            throw Exception("DebugIOGroup::push_signal(): signal_name " + signal_name +
                            " not defined for domain " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx > m_topo.num_domain(domain_type)) {
            throw Exception("DebugIOGroup::push_signal(): domain index out of bounds "
                            "for domain" + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_idx.at({signal_name, domain_idx});
    }

    int DebugIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("DebugIOGroup::push_control(): there are no controls supported by the DebugIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void DebugIOGroup::read_batch(void)
    {

    }

    void DebugIOGroup::write_batch(void)
    {

    }

    double DebugIOGroup::sample(int batch_idx)
    {
        if (batch_idx < 0 || (size_t)batch_idx >= m_value_cache->size()) {
            throw Exception("DebugIOGroup::sample(): batch_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_value_cache->at(batch_idx);
    }

    void DebugIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("DebugIOGroup::adjust(): there are no controls supported by the DebugIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double DebugIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup:read_signal(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("DebugIOGroup::read_signal(): signal_name " + signal_name +
                            " not defined for domain " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx > m_topo.num_domain(domain_type)) {
            throw Exception("DebugIOGroup::read_signal(): domain index out of bounds "
                            "for domain" + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_value_cache->at(m_signal_idx.at({signal_name, domain_idx}));
    }

    void DebugIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {

    }

    void DebugIOGroup::save_control(void)
    {

    }

    void DebugIOGroup::restore_control(void)
    {

    }

    std::string DebugIOGroup::plugin_name(void)
    {
        return "DEBUG";
    }

    std::unique_ptr<IOGroup> DebugIOGroup::make_plugin(void)
    {
        throw Exception("DebugIOGroup::make_plugin(): this IOGroup should not be created through factory.",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        return nullptr;
    }

    std::function<double(const std::vector<double> &)> DebugIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::agg_function(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return Agg::select_first;
    }

    std::string DebugIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::signal_description(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return "DebugIOGroup signals should only be used by an Agent.  "
               "No description is available.";
    }

    std::string DebugIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("DebugIOGroup::control_description(): there are no controls supported by the DebugIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        return "";
    }
}
