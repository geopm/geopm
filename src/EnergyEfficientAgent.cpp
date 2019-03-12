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

#include <sstream>
#include <cmath>
#include <iomanip>

#include "contrib/json11/json11.hpp"

#include "geopm.h"
#include "geopm_hash.h"

#include "FrequencyGovernor.hpp"
#include "EnergyEfficientRegion.hpp"
#include "EnergyEfficientAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    EnergyEfficientAgent::EnergyEfficientAgent()
        : EnergyEfficientAgent(platform_io(), platform_topo())
    {

    }

    EnergyEfficientAgent::EnergyEfficientAgent(IPlatformIO &plat_io, IPlatformTopo &topo)
        : M_PRECISION(16)
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , M_SEND_PERIOD(10)
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
        , m_num_ascend(0)
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
            for (auto sample : sample_names()) {
                m_agg_func.push_back(m_platform_io.agg_function(sample));
            }
        }
    }

    void EnergyEfficientAgent::validate_policy(std::vector<double> &policy) const
    {
#ifdef GEOPM_DEBUG
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (m_level == 0) {
            m_freq_governor->validate_policy(policy[M_POLICY_FREQ_MIN], policy[M_POLICY_FREQ_MAX]);
        }
    }

    bool EnergyEfficientAgent::update_policy(const std::vector<double> &policy)
    {
        bool result = false;
        if (m_level == 0) {
            result = m_freq_governor->set_frequency_bounds(policy[M_POLICY_FREQ_MIN], policy[M_POLICY_FREQ_MAX]);
            double freq_step = m_freq_governor->get_frequency_step();
            for (auto &eer : m_region_map) {
                eer.second->update_freq_range(policy[M_POLICY_FREQ_MIN], policy[M_POLICY_FREQ_MAX], freq_step);
            }
        }
        return result;
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

        bool result = update_policy(in_policy);
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
        return result;
    }

    bool EnergyEfficientAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                                      std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (m_num_ascend == 0) {
            aggregate_sample(in_sample, m_agg_func, out_sample);
            result = true;
        }
        ++m_num_ascend;
        if (m_num_ascend == M_SEND_PERIOD) {
           m_num_ascend = 0;
        }
        return result;
    }

    bool EnergyEfficientAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        update_policy(in_policy);
        double freq = NAN;
        double freq_min = NAN;
        double freq_max = NAN;
        m_freq_governor->get_frequency_bounds(freq_min, freq_max);
        std::vector<double> target_freq;
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            const uint64_t curr_hash = m_last_region[ctl_idx].region_hash;
            //const uint64_t curr_hint = m_last_region[ctl_idx].region_hint;
            auto freq_it = m_adapt_freq_map.find(curr_hash);
            auto reg_it = m_region_map.find(curr_hash);
            if (freq_it != m_adapt_freq_map.end() &&
                !std::isnan(reg_it->second->freq())) {
                freq = reg_it->second->freq();
            }
            else {
                double freq_step = m_freq_governor->get_frequency_step();
                freq = freq_max - freq_step;
            }
            target_freq.push_back(freq);
        }

        return m_freq_governor->adjust_platform(target_freq, m_last_freq);
    }

    bool EnergyEfficientAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        out_sample[M_SAMPLE_ENERGY_PACKAGE] = m_platform_io.sample(m_signal_idx[M_SIGNAL_ENERGY_PACKAGE][0]);
        out_sample[M_SAMPLE_FREQUENCY] = m_platform_io.sample(m_signal_idx[M_SIGNAL_FREQUENCY][0]);
        double freq_step = m_freq_governor->get_frequency_step();
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            const uint64_t current_region_hash = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HASH][ctl_idx]);
            const uint64_t current_region_hint = m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT][ctl_idx]);
            if (current_region_hash != GEOPM_REGION_HASH_INVALID) {
                bool is_region_boundary = m_last_region[ctl_idx].region_hash != current_region_hash;
                if (is_region_boundary) {
                    double freq_min = NAN;
                    double freq_max = NAN;
                    m_freq_governor->get_frequency_bounds(freq_min, freq_max);
                    if (current_region_hash != GEOPM_REGION_HASH_UNMARKED) {
                        // set the freq for the current region (entry)
                        auto curr_region_it = m_region_map.find(current_region_hash);
                        if (curr_region_it == m_region_map.end()) {
                            auto tmp = m_region_map.emplace(current_region_hash, std::unique_ptr<EnergyEfficientRegion>(
                                                            new EnergyEfficientRegion(m_platform_io,
                                                                 freq_min, freq_max, freq_step,
                                                                 m_signal_idx[M_SIGNAL_REGION_RUNTIME][ctl_idx],
                                                                 m_signal_idx[M_SIGNAL_ENERGY_PACKAGE][0])));
                            curr_region_it = tmp.first;
                        }
                        curr_region_it->second->update_entry();
                        m_adapt_freq_map[current_region_hash] = curr_region_it->second->freq();
                    }

                    // update previous region (exit)
                    uint64_t last_region = m_last_region[ctl_idx].region_hash;
                    if (last_region != GEOPM_REGION_HASH_INVALID &&
                        last_region != GEOPM_REGION_HASH_UNMARKED) {
                        auto last_region_it = m_region_map.find(last_region);
                        if (last_region_it == m_region_map.end()) {
                        throw Exception("EnergyEfficientAgent::" + std::string(__func__) + "(): region exit before entry detected.",
                                        GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                        }
                        last_region_it->second->update_exit();
                    }
                }
                m_last_region[ctl_idx].region_hash = current_region_hash;
                m_last_region[ctl_idx].region_hint = current_region_hint;
            }
        }
        return true;
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
        return {"ENERGY_PACKAGE", "FREQUENCY"};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientAgent::report_node(void) const
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
        /// "is_learning", "sticker_epoch_perf_metric", "sticker_epoch_energy_metric"
        return {};
    }

    void EnergyEfficientAgent::trace_values(std::vector<double> &values)
    {
    }

    void EnergyEfficientAgent::init_platform_io(void)
    {
        if (m_freq_governor == nullptr) {
            m_freq_governor = geopm::make_unique<FrequencyGovernor>(m_platform_io, m_platform_topo);
        }
        const int freq_ctl_domain_type = m_freq_governor->init_platform_io();
        m_num_freq_ctl_domain = m_platform_topo.num_domain(freq_ctl_domain_type);
        m_last_region = std::vector<struct geopm_region_info_s>(m_num_freq_ctl_domain,
                                                          (struct geopm_region_info_s) {.region_hash = GEOPM_REGION_HASH_INVALID,
                                                                                  .region_hint = GEOPM_REGION_HINT_UNKNOWN});
        m_last_freq = std::vector<double>(m_num_freq_ctl_domain, NAN);
        std::vector<std::string> signal_names = {"REGION_HASH", "REGION_HINT", "REGION_RUNTIME"};
        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(std::vector<int>());
            for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
                m_signal_idx[sig_idx].push_back(m_platform_io.push_signal(signal_names[sig_idx],
                                                                          freq_ctl_domain_type,
                                                                          ctl_idx));
            }
        }

        std::vector<std::string> more_signal_names = sample_names();
        for (size_t sig_idx = 0; sig_idx < more_signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back({m_platform_io.push_signal(more_signal_names[sig_idx],
                                                             IPlatformTopo::M_DOMAIN_BOARD,
                                                             0)});
        }
    }
}
