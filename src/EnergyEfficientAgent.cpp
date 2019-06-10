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
        : EnergyEfficientAgent(platform_io(), platform_topo(),
                               FrequencyGovernor::make_shared(),
                               std::map<uint64_t, std::shared_ptr<EnergyEfficientRegion> >())
    {

    }

    EnergyEfficientAgent::EnergyEfficientAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                                               std::shared_ptr<FrequencyGovernor> gov,
                                               std::map<uint64_t, std::shared_ptr<EnergyEfficientRegion> > region_map)
        : M_PRECISION(16)
        , M_WAIT_SEC(0.005)
        , M_MIN_LEARNING_RUNTIME(M_WAIT_SEC * 10)
        , M_NETWORK_NUM_SAMPLE_DELAY(2)
        , M_UNMARKED_NUM_SAMPLE_DELAY(2)
        , M_POLICY_PERF_MARGIN_DEFAULT(0.10)  // max 10% performance degradation
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_freq_governor(gov)
        , m_freq_ctl_domain_type(m_freq_governor->frequency_domain_type())
        , m_num_freq_ctl_domain(m_platform_topo.num_domain(m_freq_ctl_domain_type))
        , m_region_map(m_num_freq_ctl_domain, region_map)
        , m_samples_since_boundary(m_num_freq_ctl_domain)
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
        , m_do_send_policy(false)
        , m_perf_margin(M_POLICY_PERF_MARGIN_DEFAULT)
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

    bool EnergyEfficientAgent::update_policy(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                            "(): in_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_perf_margin = in_policy[M_POLICY_PERF_MARGIN];
        // @todo: to support dynamic policies, policy values need to be passed to regions
        return m_freq_governor->set_frequency_bounds(in_policy[M_POLICY_FREQ_MIN],
                                                     in_policy[M_POLICY_FREQ_MAX]);
    }

    void EnergyEfficientAgent::validate_policy(std::vector<double> &policy) const
    {
#ifdef GEOPM_DEBUG
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                            "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (std::isnan(policy[M_POLICY_PERF_MARGIN])) {
            policy[M_POLICY_PERF_MARGIN] = M_POLICY_PERF_MARGIN_DEFAULT;
        }
        else if (policy[M_POLICY_PERF_MARGIN] < 0.0 || policy[M_POLICY_PERF_MARGIN] > 1.0) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): performance margin must be between 0.0 and 1.0.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_freq_governor->validate_policy(policy[M_POLICY_FREQ_MIN],
                                         policy[M_POLICY_FREQ_MAX]);
    }

    void EnergyEfficientAgent::split_policy(const std::vector<double> &in_policy,
                                            std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                            "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        for (auto &child_policy : out_policy) {
            if (child_policy.size() != M_NUM_POLICY) {
                throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                                "(): child_policy vector not correctly sized.",
                                GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
            }
        }
#endif
        m_do_send_policy = update_policy(in_policy);

        if (m_do_send_policy) {
            std::fill(out_policy.begin(), out_policy.end(), in_policy);
        }
    }

    bool EnergyEfficientAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
    }

    void EnergyEfficientAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                                std::vector<double> &out_sample)
    {

    }

    bool EnergyEfficientAgent::do_write_batch(void) const
    {
        return m_freq_governor->do_write_batch();
    }

    void EnergyEfficientAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        update_policy(in_policy);
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            uint64_t hash = m_last_region_info[ctl_idx].hash;
            uint64_t hint = m_last_region_info[ctl_idx].hint;
            int samples = m_samples_since_boundary[ctl_idx];
            if (GEOPM_REGION_HASH_UNMARKED == hash) {
                if (M_UNMARKED_NUM_SAMPLE_DELAY < samples) {
                    m_target_freq[ctl_idx] = m_freq_governor->get_frequency_max();
                }
            }
            else if (GEOPM_REGION_HINT_NETWORK == hint) {
                if (M_NETWORK_NUM_SAMPLE_DELAY < samples) {
                    m_target_freq[ctl_idx] = m_freq_governor->get_frequency_min();
                }
            }
            else {
                auto region_it = m_region_map[ctl_idx].find(hash);
                if (region_it != m_region_map[ctl_idx].end()) {
                    m_target_freq[ctl_idx] = region_it->second->freq();
                }
                else {
                    throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                                    "(): unknown target frequency hash = " + std::to_string(hash),
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
            }
        }
        m_freq_governor->adjust_platform(m_target_freq);
    }

    void EnergyEfficientAgent::sample_platform(std::vector<double> &out_sample)
    {
        double freq_min = m_freq_governor->get_frequency_min();
        double freq_max = m_freq_governor->get_frequency_max();
        double freq_step = m_freq_governor->get_frequency_step();
        std::set<std::pair<uint64_t, int> > exit_set;
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            struct m_region_info_s current_region_info {
                .hash = (uint64_t)m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH][ctl_idx]),
                .hint = (uint64_t)m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT][ctl_idx]),
                .runtime = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_RUNTIME][ctl_idx]),
                .count = (uint64_t)m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_COUNT][ctl_idx])};
            // If region hash has changed, or region count changed for the same region
            // update current region (entry)
            if (m_last_region_info[ctl_idx].hash != current_region_info.hash ||
                m_last_region_info[ctl_idx].count != current_region_info.count) {
                m_samples_since_boundary[ctl_idx] = 0;
                if (current_region_info.hash != GEOPM_REGION_HASH_UNMARKED &&
                    current_region_info.hint != GEOPM_REGION_HINT_NETWORK) {
                    /// set the freq for the current region (entry)
                    auto current_region_it = m_region_map[ctl_idx].emplace(std::piecewise_construct,
                                                                           std::forward_as_tuple(current_region_info.hash),
                                                                           std::forward_as_tuple(geopm::make_unique<EnergyEfficientRegionImp>
                                                                                                 (freq_min, freq_max, freq_step, m_perf_margin))).first;
                    // Higher is better for performance, so negate
                    current_region_it->second->sample(-1.0 * current_region_info.runtime);
                }
                /// update previous region (exit)
                struct m_region_info_s last_region_info = m_last_region_info[ctl_idx];
                if (last_region_info.hash != GEOPM_REGION_HASH_UNMARKED &&
                    last_region_info.hint != GEOPM_REGION_HINT_NETWORK) {
                    auto last_region_it = m_region_map[ctl_idx].find(last_region_info.hash);
                    if (last_region_it == m_region_map[ctl_idx].end()) {
                        throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                                        "(): region exit before entry detected.",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                    }
                    if (last_region_info.runtime != 0.0 &&
                        last_region_info.runtime < M_MIN_LEARNING_RUNTIME) {
                        last_region_it->second->disable();
                    }
                    exit_set.insert(std::make_pair(last_region_info.hash, ctl_idx));
                }
                m_last_region_info[ctl_idx] = current_region_info;
            }
            else {
                ++m_samples_since_boundary[ctl_idx];
            }
        }
        for (auto &exit_pair : exit_set) {
            uint64_t exit_hash = exit_pair.first;
            int ctl_idx = exit_pair.second;
            auto it = m_region_map[ctl_idx].find(exit_hash);
            if (it == m_region_map[ctl_idx].end()) {
                throw Exception("EnergyEfficientAgent::" + std::string(__func__) +
                                "(): region exit before entry detected.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            it->second->calc_next_freq();
        }
    }

    bool EnergyEfficientAgent::do_send_sample(void) const
    {
        return false;
    }

    void EnergyEfficientAgent::wait(void)
    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> EnergyEfficientAgent::policy_names(void)
    {
        return {"FREQ_MIN", "FREQ_MAX", "PERF_MARGIN"};
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
        auto region_freq_map = report_region();
        std::vector<std::pair<std::string, std::string> > result;
        std::ostringstream oss;
        oss << std::setprecision(EnergyEfficientAgent::M_PRECISION) << std::scientific;
        for (const auto &region : region_freq_map) {
            oss << "\n    0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed;
            oss << region.first;
            oss << std::setfill('\0') << std::setw(0) << std::scientific;
            oss << ": " << region.second[0].second;  // Only item in the vector is requested frequency
        }
        oss << "\n";
        result.push_back({"Final online freq map", oss.str()});

        return result;
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > EnergyEfficientAgent::report_region(void) const
    {
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > result;
        std::map<uint64_t, std::pair<double, double> > region_frequency_count_map;
        for (const auto &region_map_it : m_region_map) {
            // If region is in this map, online learning was used to set frequency
            for (const auto &region : region_map_it) {
                if (!region.second->is_learning()) {
                    auto it = region_frequency_count_map.find(region.first);
                    if (it == region_frequency_count_map.end()) {
                        region_frequency_count_map[region.first] = std::make_pair(region.second->freq(), 1.0);
                    }
                    else {
                        it->second.first += region.second->freq();
                        it->second.second += 1.0;
                    }
                }
            }
        }
        for (const auto &it : region_frequency_count_map) {
            /// @todo re-implement with m_region_map and m_hash_freq_map keys as pair (hash + hint)
            // Average frequencies for all domains that completed learning
            double requested_freq = it.second.first / it.second.second;
            result[it.first].push_back(std::make_pair("requested-online-frequency",
                                                      std::to_string(requested_freq)));
        }
        return result;
    }

    std::vector<std::string> EnergyEfficientAgent::trace_names(void) const
    {
        return {};
    }

    std::vector<std::function<std::string(double)> > EnergyEfficientAgent::trace_formats(void) const
    {
        return {};
    }

    void EnergyEfficientAgent::trace_values(std::vector<double> &values)
    {

    }

    void EnergyEfficientAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_platform_io.write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, policy[M_POLICY_FREQ_MAX]);
    }

    void EnergyEfficientAgent::init_platform_io(void)
    {
        m_freq_governor->init_platform_io();
        const struct m_region_info_s DEFAULT_REGION { .hash = GEOPM_REGION_HASH_UNMARKED,
                                                      .hint = GEOPM_REGION_HINT_UNKNOWN,
                                                      .runtime = 0.0,
                                                      .count = 0};
        m_last_region_info = std::vector<struct m_region_info_s>(m_num_freq_ctl_domain, DEFAULT_REGION);
        m_target_freq.resize(m_num_freq_ctl_domain, m_freq_governor->get_frequency_max());
        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT", "REGION_RUNTIME", "REGION_COUNT"};

        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(std::vector<int>());
            for (int ctl_idx = 0; ctl_idx < m_num_freq_ctl_domain; ++ctl_idx) {
                m_signal_idx[sig_idx].push_back(m_platform_io.push_signal(signal_names[sig_idx],
                            m_freq_ctl_domain_type, ctl_idx));
            }
        }
    }
}
