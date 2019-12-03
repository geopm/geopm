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

#include "EnergyEfficientEpochAgent.hpp"

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

double geopm_agent_get_wait_env();

namespace geopm
{
    EnergyEfficientEpochAgent::EnergyEfficientEpochAgent()
        : EnergyEfficientEpochAgent(platform_io(), platform_topo(),
                               FrequencyGovernor::make_shared())
    {

    }

    EnergyEfficientEpochAgent::EnergyEfficientEpochAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                                               std::shared_ptr<FrequencyGovernor> gov)
        : M_PRECISION(16)
        , M_WAIT_SEC(geopm_agent_get_wait_env())
        , M_MIN_LEARNING_RUNTIME(M_WAIT_SEC * 10)
        , M_NETWORK_NUM_SAMPLE_DELAY(2)
        , M_POLICY_PERF_MARGIN_DEFAULT(0.10)  // max 10% performance degradation
        , m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_freq_governor(gov)
        , m_freq_ctl_domain_type(m_freq_governor->frequency_domain_type())
        , m_samples_since_boundary(m_num_freq_ctl_domain)
        , m_num_freq_ctl_domain(m_platform_topo.num_domain(m_freq_ctl_domain_type))
        , m_last_wait(GEOPM_TIME_REF)
        , m_level(-1)
        , m_num_children(0)
        , m_do_send_policy(false)
        , m_perf_margin(M_POLICY_PERF_MARGIN_DEFAULT)
    {

    }

    std::string EnergyEfficientEpochAgent::plugin_name(void)
    {
        return "energy_efficient_epoch";
    }

    std::unique_ptr<Agent> EnergyEfficientEpochAgent::make_plugin(void)
    {
        return geopm::make_unique<EnergyEfficientEpochAgent>();
    }

    void EnergyEfficientEpochAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
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

    bool EnergyEfficientEpochAgent::update_policy(const std::vector<double> &in_policy)
    {
#ifdef GEOPM_DEBUG
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientEpochAgent::" + std::string(__func__) +
                            "(): in_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        m_perf_margin = in_policy[M_POLICY_PERF_MARGIN];
        // @todo: to support dynamic policies, policy values need to be passed to regions
        return m_freq_governor->set_frequency_bounds(in_policy[M_POLICY_FREQ_MIN],
                                                     in_policy[M_POLICY_FREQ_MAX]);
    }

    void EnergyEfficientEpochAgent::validate_policy(std::vector<double> &policy) const
    {
#ifdef GEOPM_DEBUG
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientEpochAgent::" + std::string(__func__) +
                            "(): policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (std::isnan(policy[M_POLICY_PERF_MARGIN])) {
            policy[M_POLICY_PERF_MARGIN] = M_POLICY_PERF_MARGIN_DEFAULT;
        }
        else if (policy[M_POLICY_PERF_MARGIN] < 0.0 || policy[M_POLICY_PERF_MARGIN] > 1.0) {
            throw Exception("EnergyEfficientEpochAgent::" + std::string(__func__) + "(): performance margin must be between 0.0 and 1.0.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_freq_governor->validate_policy(policy[M_POLICY_FREQ_MIN],
                                         policy[M_POLICY_FREQ_MAX]);

        if (std::isnan(policy[M_POLICY_FREQ_FIXED])) {
            policy[M_POLICY_FREQ_FIXED] = m_platform_io.read_signal("FREQUENCY_MAX",
                                                                    GEOPM_DOMAIN_BOARD, 0);
        }
    }

    void EnergyEfficientEpochAgent::split_policy(const std::vector<double> &in_policy,
                                            std::vector<std::vector<double> > &out_policy)
    {
#ifdef GEOPM_DEBUG
        if (out_policy.size() != (size_t) m_num_children) {
            throw Exception("EnergyEfficientEpochAgent::" + std::string(__func__) +
                            "(): out_policy vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        for (auto &child_policy : out_policy) {
            if (child_policy.size() != M_NUM_POLICY) {
                throw Exception("EnergyEfficientEpochAgent::" + std::string(__func__) +
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

    bool EnergyEfficientEpochAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
    }

    void EnergyEfficientEpochAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                                std::vector<double> &out_sample)
    {

    }

    bool EnergyEfficientEpochAgent::do_write_batch(void) const
    {
        return m_freq_governor->do_write_batch();
    }

    void EnergyEfficientEpochAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        static bool once = false;
        update_policy(in_policy);
        if (!once) {
            once = true;
            const double freq_min = m_freq_governor->get_frequency_min();
            const double freq_max = m_freq_governor->get_frequency_max();
            const double freq_step = m_freq_governor->get_frequency_step();
            m_platform_io.write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, freq_max);
            const struct m_epoch_info_s DEFAULT_EPOCH { .count = -1,  // epoch count starts at -1
                                                        .ee_region = std::make_shared<EnergyEfficientRegionImp>(freq_min, freq_max, freq_step, m_perf_margin)};
            m_last_epoch_info.resize(m_num_freq_ctl_domain, DEFAULT_EPOCH);
            m_target_freq.resize(m_num_freq_ctl_domain, freq_max);
        }
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
            uint64_t hint = (uint64_t) m_platform_io.sample(m_signal_idx[M_SIGNAL_REGION_HINT][ctl_idx]);
            if (GEOPM_REGION_HINT_NETWORK == hint) {
                if (M_NETWORK_NUM_SAMPLE_DELAY < m_samples_since_boundary[ctl_idx]) {
                    m_target_freq[ctl_idx] = m_freq_governor->get_frequency_min();
                }
            }
            // todo maybe a case for ignored regions here
            else {
                m_target_freq[ctl_idx] = m_last_epoch_info[ctl_idx].ee_region->freq();
            }
        }
        m_freq_governor->adjust_platform(m_target_freq);
    }

    void EnergyEfficientEpochAgent::sample_platform(std::vector<double> &out_sample)
    {
        for (size_t ctl_idx = 0; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
                int64_t epoch_count = (int64_t) m_platform_io.sample(m_signal_idx[M_SIGNAL_EPOCH_COUNT][ctl_idx]);
                if (epoch_count != m_last_epoch_info[ctl_idx].count) {
                    m_samples_since_boundary[ctl_idx] = 0;
                    // todo epoch to region conversion
                    //if (m_seen_first_epoch[rank]) {
                        //record_exit(GEOPM_REGION_ID_EPOCH, rank, epoch_time);
                    //}
                    // do the eeregion equiv of record entry
                    // removing ignore time because... it is to be ignored.
                    // removing network time as we are flooring frequency when observed
                    double epoch_runtime = m_platform_io.sample(m_signal_idx[M_SIGNAL_EPOCH_RUNTIME][ctl_idx]) -
                                           m_platform_io.sample(m_signal_idx[M_SIGNAL_EPOCH_RUNTIME_NETWORK][ctl_idx]) -
                                           m_platform_io.sample(m_signal_idx[M_SIGNAL_EPOCH_RUNTIME_IGNORE][ctl_idx]);
                    //if (m_last_epoch_info.runtime != 0.0 &&
                        //m_last_epoch_info.runtime < M_MIN_LEARNING_RUNTIME) {
                        //m_last_epoch_info.ee_region->disable();
                    //}
                    // Higher is better for performance, so negate
                    //m_last_epoch_info[ctl_idx].ee_region->sample(-1.0 * epoch_runtime);
                    m_last_epoch_info[ctl_idx].ee_region->update_exit(-1.0 * epoch_runtime);
                }
                else {
                    m_samples_since_boundary[ctl_idx]++;
                }
                m_last_epoch_info[ctl_idx].count = epoch_count;
            }
    }

    bool EnergyEfficientEpochAgent::do_send_sample(void) const
    {
        return false;
    }

    void EnergyEfficientEpochAgent::wait(void)
    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> EnergyEfficientEpochAgent::policy_names(void)
    {
        return {"FREQ_MIN", "FREQ_MAX", "PERF_MARGIN", "FREQ_FIXED"};
    }

    std::vector<std::string> EnergyEfficientEpochAgent::sample_names(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientEpochAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > EnergyEfficientEpochAgent::report_host(void) const
    {
        // todo refactor in fashion such that Agg::average() can be used
        //      lamda maybe?
        // todo does it matter if learning completes for the epoch?
        //      should it affect this report aug?
        double avg_freq = m_last_epoch_info[0].ee_region->freq();
        for (size_t ctl_idx = 1; ctl_idx < (size_t) m_num_freq_ctl_domain; ++ctl_idx) {
                avg_freq = (avg_freq + m_last_epoch_info[ctl_idx].ee_region->freq()) / 2.0;
        }
        return {std::make_pair("epoch_frequency", std::to_string(avg_freq))};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > EnergyEfficientEpochAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> EnergyEfficientEpochAgent::trace_names(void) const
    {
        return {};
    }

    std::vector<std::function<std::string(double)> > EnergyEfficientEpochAgent::trace_formats(void) const
    {
        return {};
    }

    void EnergyEfficientEpochAgent::trace_values(std::vector<double> &values)
    {

    }

    void EnergyEfficientEpochAgent::enforce_policy(const std::vector<double> &policy) const
    {
        if (policy.size() != M_NUM_POLICY) {
            throw Exception("EnergyEfficientEpochAgent::enforce_policy(): policy vector incorrectly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_platform_io.write_control("FREQUENCY", GEOPM_DOMAIN_BOARD, 0, policy[M_POLICY_FREQ_FIXED]);
    }

    void EnergyEfficientEpochAgent::init_platform_io(void)
    {
        m_freq_governor->init_platform_io();
        std::vector<std::string> signal_names = {"EPOCH_COUNT", "EPOCH_RUNTIME", "EPOCH_RUNTIME_NETWORK", "EPOCH_RUNTIME_IGNORE",
                                                 "REGION_HASH", "REGION_HINT", "REGION_RUNTIME", "REGION_COUNT"};

        for (size_t sig_idx = 0; sig_idx < signal_names.size(); ++sig_idx) {
            m_signal_idx.push_back(std::vector<int>());
            for (int ctl_idx = 0; ctl_idx < m_num_freq_ctl_domain; ++ctl_idx) {
                m_signal_idx[sig_idx].push_back(m_platform_io.push_signal(signal_names[sig_idx],
                            m_freq_ctl_domain_type, ctl_idx));
            }
        }
    }
}
