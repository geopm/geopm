/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include "TelemetryConfig.hpp"
#include "Exception.hpp"

namespace geopm
{

    TelemetryConfig::TelemetryConfig(const std::vector<int> &fanout)
        : m_fan_out(fanout)
    {

    }

    TelemetryConfig::TelemetryConfig(const TelemetryConfig &other)
        : m_provided_signal(other.m_provided_signal)
        , m_required_signal(other.m_required_signal)
        , m_aggregate_signal(other.m_aggregate_signal)
        , m_control_bound(other.m_control_bound)
        , m_domain_map(other.m_domain_map)
        , m_supported_domain(other.m_supported_domain)
        , m_fan_out(other.m_fan_out)
    {

    }

    TelemetryConfig::~TelemetryConfig()
    {

    }

    void TelemetryConfig::set_provided(int signal_domain, const std::vector<std::string> &provided)
    {
        auto entry = m_provided_signal.find(signal_domain);
        if (entry != m_provided_signal.end()) {
            entry->second.insert(entry->second.end(), provided.begin(), provided.end());
        }
        else {
            m_provided_signal.insert(std::pair<int, std::vector<std::string> >(signal_domain, {provided}));
        }
    }

    void TelemetryConfig::get_provided(int signal_domain, std::vector<std::string> &provided) const
    {
        auto it = m_provided_signal.find(signal_domain);
        if (it == m_provided_signal.end()) {
            provided.clear();
        }
        provided = (*it).second;
    }

    bool TelemetryConfig::is_provided(int signal_domain, const std::string &signal) const
    {
        bool rval = false;
        auto it = m_provided_signal.find(signal_domain);
        if (it != m_provided_signal.end()) {
            for (auto sig_it = it->second.begin(); sig_it != it->second.end(); ++sig_it) {
                if ((*sig_it) == signal) {
                    rval = true;
                    break;
                }
            }
        }
        return rval;
    }

    void TelemetryConfig::get_required(int signal_domain, std::vector<std::string> &required) const
    {
        auto it = m_required_signal.find(signal_domain);
        if (it == m_required_signal.end()) {
            required.clear();
        }
        required = (*it).second;;
    }

    void TelemetryConfig::get_required(std::map<int, std::vector<std::string> > &required) const
    {
        required = m_required_signal;
    }

    void TelemetryConfig::set_required(int signal_domain, const std::vector<std::string> &required)
    {
        auto entry = m_required_signal.find(signal_domain);
        if (entry != m_required_signal.end()) {
            (*entry).second.insert((*entry).second.end(), required.begin(), required.end());
        }
        else {
            m_required_signal.insert(std::pair<int, std::vector<std::string> >(signal_domain, required));
        }
    }

    void TelemetryConfig::set_required(int signal_domain, const std::string &required)
    {
        auto entry = m_required_signal.find(signal_domain);
        if (entry != m_required_signal.end()) {
            (*entry).second.push_back(required);
        }
        else {
            m_required_signal.insert(std::pair<int, std::vector<std::string> >(signal_domain, {required}));
        }
    }

    bool TelemetryConfig::is_required(int signal_domain, const std::string &signal) const
    {
        bool rval = false;
        auto entry = m_required_signal.find(signal_domain);
        if (entry != m_required_signal.end()) {
            for (auto sig_it = entry->second.begin(); sig_it != entry->second.end(); ++sig_it) {
                if ((*sig_it) == signal) {
                    rval = true;
                    break;
                }
            }
        }
        return rval;
    }

    void TelemetryConfig::set_aggregate(const std::vector<std::pair<std::string, std::pair<int, int> > > &agg)
    {
        m_aggregate_signal.insert(m_aggregate_signal.end(), agg.begin(), agg.end());
    }

    void TelemetryConfig::set_aggregate(std::string signal, int spacial_op_type, int temporal_op_type)
    {
        std::pair<int, int> ops(spacial_op_type, temporal_op_type);
        m_aggregate_signal.push_back(std::pair<std::string, std::pair<int, int> >(signal, ops));
    }

    void TelemetryConfig::get_aggregate(std::vector<std::pair<std::string, std::pair<int, int> > > &agg) const
    {
        agg = m_aggregate_signal;
    }

    int TelemetryConfig::num_aggregated_signal(void) const
    {
        return m_aggregate_signal.size();
    }
    void TelemetryConfig::get_domain_cpu_map(int domain, std::vector<std::vector<int> > &domain_map) const
    {
        auto map = m_domain_map.find(domain);
        if (map == m_domain_map.end()) {
            throw Exception("TelemetryConfig::get_domain_cpu_map(): unknown domain: " +
                            std::to_string(domain),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        domain_map = (*map).second;
    }

    void TelemetryConfig::set_domain_cpu_map(int domain, const std::vector<std::vector<int> > &domain_map)
    {
        auto map = m_domain_map.find(domain);
        if (map == m_domain_map.end()) {
            m_domain_map.insert(std::pair<int, std::vector<std::vector<int> > >(domain,  domain_map));
        }
        else{
            throw Exception("TelemetryConfig::set_domain_cpu_map(): domain map already exists: ",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    int TelemetryConfig::num_signal_per_domain(int signal_domain) const
    {
        auto map = m_domain_map.find(signal_domain);
        if (map == m_domain_map.end()) {
            throw Exception("TelemetryConfig::num_signal_per_domain(): unknown domain: " +
                            std::to_string(signal_domain),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return map->second.size();
    }

    void TelemetryConfig::num_signal_per_domain(std::vector<int>  &num_signal) const
    {
        num_signal.clear();
        for (auto req_it = m_required_signal.begin(); req_it != m_required_signal.end(); ++req_it) {
            num_signal.push_back(num_signal_per_domain(req_it->first));
        }
    }

    int TelemetryConfig::num_required_signal(void) const
    {
        int signum = 0;
        for (auto& it : m_required_signal) {
            signum += it.second.size() * num_signal_per_domain(it.first);
        }
        return signum;
    }

    void TelemetryConfig::set_bounds(int control_domain, double lower, double upper)
    {
        auto bounds = m_control_bound.find(control_domain);
        if (bounds == m_control_bound.end()) {
            bounds->second.first = lower;
            bounds->second.second = upper;
        }
        else {
            std::pair<int, int> bound(lower, upper);
            m_control_bound.insert(std::pair<int, std::pair<int, int> >(control_domain, bound));
        }
    }

    void TelemetryConfig::get_bounds(int level, int control_type, double &lower, double &upper) const
    {
        auto bounds = m_control_bound.find(control_type);
        if (bounds == m_control_bound.end()) {
            throw Exception("TelemetryConfig::bounds(): unknown control type: " +
                            std::to_string(control_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        lower = (*bounds).second.first;
        upper = (*bounds).second.second;
        for (int i = 0; i < level; ++i) {
            lower *= m_fan_out[i];
            upper *= m_fan_out[i];
        }
    }

    void TelemetryConfig::supported_domain(const std::vector<int> domain)
    {
        m_supported_domain = domain;
    }

    bool TelemetryConfig::is_supported_domain(int domain) const
    {
        bool rval= false;
        for (auto it = m_supported_domain.begin(); it != m_supported_domain.end(); ++it) {
            if ((*it) == domain) {
                rval = true;
                break;
            }
        }
        return rval;
    }
}
