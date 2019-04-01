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

#include "EnergyEfficientAgent.hpp"

#include <sstream>
#include <cmath>
#include <iomanip>
#include <utility>

#include "contrib/json11/json11.hpp"

#include "geopm.h"
#include "geopm_hash.h"

#include "EnergyEfficientRegion.hpp"
#include "FrequencyGovernor.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    EnergyEfficientAgent::EnergyEfficientAgent()
        : EnergyEfficientAgent(platform_io(), platform_topo(), nullptr)
    {

    }

    EnergyEfficientAgent::EnergyEfficientAgent(PlatformIO &plat_io, PlatformTopo &topo,
                                               std::unique_ptr<FrequencyGovernor> gov)
        : M_PRECISION(16)
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_freq_governor(std::move(gov))
        , m_num_freq_ctl_domain(-1)
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
    {

    }

    std::string EnergyEfficientAgent::plugin_name(void)
    {
        return "energy_efficient";
    }

    std::unique_ptr<Agent> EnergyEfficientAgent::make_plugin(void)
    {
        return geopm::make_unique<EnergyEfficientAgent>();
    }

    void EnergyEfficientAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_level = level;
        if (m_level == 0) {
            m_num_children = 0;
            init_platform_io();
        }
        else {
            m_num_children = fan_in[level - 1];
        }
    }

    bool EnergyEfficientAgent::update_freq_range(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): in_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = m_freq_governor->set_frequency_bounds(in_policy[M_POLICY_FREQ_MIN], in_policy[M_POLICY_FREQ_MAX]);;
        if (result) {
            for (auto &eer : m_region_map) {
                double freq_min = NAN;
                double freq_max = NAN;
                double freq_step = NAN;
                m_freq_governor->get_frequency_bounds(freq_min, freq_max, freq_step);
                eer.second->update_freq_range(freq_min, freq_max, freq_step);
            }
            result = true;
        }
        return result;
    }

    void EnergyEfficientAgent::validate_policy(std::vector<double> &policy) const
    {
#ifdef GEOPM_DEBUG
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_freq_governor->validate_policy(policy[M_POLICY_FREQ_MIN], policy[M_POLICY_FREQ_MAX]);
    }

    bool EnergyEfficientAgent::descend(const std::vector<double> &in_policy,
                                       std::vector<std::vector<double> >&out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = update_freq_range(in_policy);

        if (result) {
            for (auto &child_policy : out_policy) {
#ifdef GEOPM_DEBUG
                if (child_policy.size() != M_NUM_POLICY) {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): child_policy vector not correctly sized.",
                                    GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
                }
#endif
                child_policy[M_POLICY_FREQ_MIN] = in_policy[M_POLICY_FREQ_MIN];
                child_policy[M_POLICY_FREQ_MAX] = in_policy[M_POLICY_FREQ_MAX];
            }
        }
        return result;
    }

    bool EnergyEfficientAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                                      std::vector<double> &out_sample)
    {
        return false;
    }

    bool EnergyEfficientAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        bool result = false;
        update_freq_range(in_policy);
        std::vector<double> target_freq;
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            auto it = m_adapt_freq_map.find(m_last_region[ctl_idx].hash);
            if (GEOPM_REGION_HASH_INVALID != m_last_region[ctl_idx].hash &&
                it != m_adapt_freq_map.end()) {
                target_freq.push_back(m_adapt_freq_map[m_last_region[ctl_idx].hash]);
            }
            else if (GEOPM_REGION_HASH_INVALID != m_last_region[ctl_idx].hash) {
                throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): unknown target frequency.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        if (target_freq.size()) {
            result = m_freq_governor->adjust_platform(target_freq, m_last_freq);
        }
        return result;
    }

    bool EnergyEfficientAgent::sample_platform(std::vector<double> &out_sample)
    {
        double freq_min = NAN;
        double freq_max = NAN;
        double freq_step = NAN;
        m_freq_governor->get_frequency_bounds(freq_min, freq_max, freq_step);
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            const uint64_t current_region_hash = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH][ctl_idx]);
            const uint64_t current_region_hint = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT][ctl_idx]);
            const double current_region_runtime = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_RUNTIME][ctl_idx]);
            const bool is_region_boundary = m_last_region[ctl_idx].hash != current_region_hash;
            /// update current region (entry)
            if (is_region_boundary) {
                const bool curr_region_is_invalid = current_region_hash == GEOPM_REGION_HASH_INVALID;
                double target_freq = NAN;
                if (current_region_hint == GEOPM_REGION_HINT_COMPUTE ||
                    current_region_hint == GEOPM_REGION_HASH_UNMARKED) {
                    /// Compute regions are assigned maximum frequency, no learning
                    target_freq = freq_max;
                }
                else if (!curr_region_is_invalid) {
                    /// set the freq for the current region (entry)
                    auto curr_region_it = m_region_map.find(current_region_hash);
                    if (curr_region_it == m_region_map.end()) {
                        auto tmp = m_region_map.emplace(current_region_hash, geopm::make_unique<EnergyEfficientRegion>
                                                        (freq_min, freq_max, freq_step));
                        curr_region_it = tmp.first;
                    }
                    target_freq = curr_region_it->second->freq();
                }
                else {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): unexpected (region hash:hint)",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                m_adapt_freq_map[current_region_hash] = target_freq;
            }
            /// update previous region (exit)
            if (is_region_boundary) {
                const uint64_t last_region_hash = m_last_region[ctl_idx].hash;
                const uint64_t last_region_hint = m_last_region[ctl_idx].hint;
                // Higher is better for performance, so negate
                const double last_region_perf_metric = -1.0 * m_last_region[ctl_idx].runtime;
                const bool curr_region_is_invalid = last_region_hash == GEOPM_REGION_HASH_INVALID;
                if (last_region_hash != GEOPM_REGION_HASH_INVALID &&
                    last_region_hash != GEOPM_REGION_HASH_UNMARKED &&
                    last_region_hint != GEOPM_REGION_HINT_COMPUTE) {
                    auto last_region_it = m_region_map.find(last_region_hash);
                    if (last_region_it == m_region_map.end()) {
                        throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): region exit before entry detected.",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                    last_region_it->second->update_exit(last_region_perf_metric);
                }
                m_last_region[ctl_idx] = {current_region_hash, current_region_hint, 0, current_region_runtime};
            }
        }
        return false;
    }

    void EnergyEfficientAgent::wait(void)
    {
        static double M_WAIT_SEC = 0.005;
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> EnergyEfficientAgent::policy_names(void)
    {
        return {"FREQ_MIN", "FREQ_MAX"};
    }

    std::vector<std::string> EnergyEfficientAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_host(void) const
    {
        std::vector<std::pair<std::string, std::string> > result;
        std::ostringstream oss;
        oss << std::setprecision(EnergyEfficientAgent::M_PRECISION) << std::scientific;
        for (const auto &region : m_region_map) {
            oss << "\n\t0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed;
            oss << region.first;
            oss << std::setfill('\0') << std::setw(0) << std::scientific;
            oss << ":" << region.second->freq();
        }
        oss << "\n";
        result.push_back({"Final online freq map", oss.str()});

        return result;
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > EnergyEfficientAgent::report_region(void) const
    {
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > result;
        // If region is in this map, online learning was used to set frequency
        for (const auto &region : m_region_map) {
            /// @todo re-implement with m_region_map and m_hash_freq_map keys as pair (hash + hint)
            result[region.first].push_back(std::make_pair("requested-online-frequency", std::to_string(region.second->freq())));
        }
        return result;
    }

    std::vector<std::string> EnergyEfficientAgent::trace_names(void) const
    {
        return {};
    }

    void EnergyEfficientAgent::trace_values(std::vector<double> &values)
    {
    }

    void EnergyEfficientAgent::init_platform_io(void)
    {
        if (m_freq_governor == nullptr) {
            m_freq_governor = FrequencyGovernor::make_unique();
        }
        m_freq_governor->init_platform_io();
        const int freq_ctl_domain_type = m_freq_governor->frequency_domain_type();
        m_num_freq_ctl_domain = m_platform_topo.num_domain(freq_ctl_domain_type);
        m_last_region = std::vector<struct geopm_region_info_s>(m_num_freq_ctl_domain,
                                                          (struct geopm_region_info_s) {
                                                          .hash = GEOPM_REGION_HASH_INVALID,
                                                          .hint = GEOPM_REGION_HINT_UNKNOWN,
                                                          .progress = 0,
                                                          .runtime = 0});
        m_last_freq = std::vector<double>(m_num_freq_ctl_domain, NAN);

        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT", "REGION_RUNTIME"};

        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(std::vector<int>());
            for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
                m_signal_idx[sig_idx].push_back(m_platform_io.push_signal(signal_names[sig_idx],
                            freq_ctl_domain_type, ctl_idx));
            }
        }
    }
}
