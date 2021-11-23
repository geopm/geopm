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

#include "FixedFrequencyAgent.hpp"

#include <iostream>

#include <cmath>
#include <cassert>
#include <algorithm>

#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"

#include <string>

namespace geopm
{
    FixedFrequencyAgent::FixedFrequencyAgent():
        FixedFrequencyAgent(platform_io(), platform_topo())
    {
    }

    FixedFrequencyAgent::FixedFrequencyAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(platform_topo())
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(0.05) // 50mS wait
        , m_do_write_batch(false)
        , m_is_adjust_initialized(false)
        , m_signal_available({{"FREQUENCY_ACCELERATOR", {}},
                              {"POWER_ACCELERATOR", {}},
                              {"NVML::TOTAL_ENERGY_CONSUMPTION", {}},
                              {"MSR::UNCORE_PERF_STATUS:FREQ", {}},
                              {"FREQUENCY", {}},
    					     })
        , m_control_available({{"FREQUENCY_ACCELERATOR_CONTROL", {}},
                               {"CPU_FREQUENCY_CONTROL", {}},
                               {"MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", {}},
                               {"MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", {}},
                              })
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void FixedFrequencyAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        if (level == 0) {
            init_platform_io();
        }
    }

    void FixedFrequencyAgent::init_platform_io(void)
    {
        // Populate signals for each domain with batch idx info, default values, etc
        for (auto &sv : m_signal_available) {
            sv.second.m_batch_idx = m_platform_io.push_signal(sv.first, GEOPM_DOMAIN_BOARD, 0);
        }

        // Populate controls for each domain
        for (auto &sv : m_control_available) {
            sv.second.m_batch_idx = m_platform_io.push_control(sv.first, GEOPM_DOMAIN_BOARD, 0);
        }

    }

    // Validate incoming policy and configure default policy requests.
    void FixedFrequencyAgent::validate_policy(std::vector<double> &in_policy) const
    {
        assert(in_policy.size() == M_NUM_POLICY);
    }

    // Distribute incoming policy to children
    void FixedFrequencyAgent::split_policy(const std::vector<double>& in_policy,
                                    std::vector<std::vector<double> >& out_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool FixedFrequencyAgent::do_send_policy(void) const
    {
        return true;
    }

    void FixedFrequencyAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double>& out_sample)
    {

    }

    // Indicate whether to send samples up to the parent
    bool FixedFrequencyAgent::do_send_sample(void) const
    {
        return false;
    }

    void FixedFrequencyAgent::adjust_platform(const std::vector<double>& in_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);
        m_do_write_batch = false;

        double accel_freq_request = in_policy[M_POLICY_ACCELERATOR_FREQUENCY];
        double cpu_freq_request = in_policy[M_POLICY_CPU_FREQUENCY];
        double uncore_freq_request = in_policy[M_POLICY_UNCORE_FREQUENCY];

        // set Accelerator frequency control
        auto freq_ctl_itr = m_control_available.find("FREQUENCY_ACCELERATOR_CONTROL");
        if (!std::isnan(in_policy[M_POLICY_ACCELERATOR_FREQUENCY])) {
            if (accel_freq_request != freq_ctl_itr->second.m_last_setting) {
                m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, accel_freq_request);
                freq_ctl_itr->second.m_last_setting = accel_freq_request;
                m_do_write_batch = true;
            }
        }

        // set CPU frequency control
        freq_ctl_itr = m_control_available.find("CPU_FREQUENCY_CONTROL");
        if (!std::isnan(in_policy[M_POLICY_CPU_FREQUENCY])) {
            if (cpu_freq_request != freq_ctl_itr->second.m_last_setting) {
                m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, cpu_freq_request);
                freq_ctl_itr->second.m_last_setting = cpu_freq_request;
                m_do_write_batch = true;
            }
        }

        // set Uncore frequency control
        freq_ctl_itr = m_control_available.find("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO");
        if (!std::isnan(in_policy[M_POLICY_UNCORE_FREQUENCY])) {
            if (uncore_freq_request != freq_ctl_itr->second.m_last_setting) {
                m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, uncore_freq_request);
                freq_ctl_itr->second.m_last_setting = uncore_freq_request;
                m_do_write_batch = true;
            }
            freq_ctl_itr = m_control_available.find("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO");
            if (uncore_freq_request != freq_ctl_itr->second.m_last_setting) {
                m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, uncore_freq_request);
                freq_ctl_itr->second.m_last_setting = uncore_freq_request;
                m_do_write_batch = true;
            }
        }

        if (!m_is_adjust_initialized) {
            double accel_init_freq = m_platform_io.read_signal("NVML::FREQUENCY_MAX", GEOPM_DOMAIN_BOARD, 0);
            freq_ctl_itr = m_control_available.find("FREQUENCY_ACCELERATOR_CONTROL");
            m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, accel_init_freq);
            freq_ctl_itr->second.m_last_setting = accel_init_freq;

            double cpu_init_freq = m_platform_io.read_signal("CPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0);
            freq_ctl_itr = m_control_available.find("CPU_FREQUENCY_CONTROL");
            m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, cpu_init_freq);
            freq_ctl_itr->second.m_last_setting = cpu_init_freq;

            double uncore_init_min = m_platform_io.read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0);
            freq_ctl_itr = m_control_available.find("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO");
            m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, uncore_init_min);

            double uncore_init_max = m_platform_io.read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0);
            freq_ctl_itr = m_control_available.find("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO");
            m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, uncore_init_max);

            m_is_adjust_initialized = true;
            m_do_write_batch = true;
        }

    }

    //If new values have been adjusted, write
    bool FixedFrequencyAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    // Read signals from the platform and calculate samples to be sent up
    void FixedFrequencyAgent::sample_platform(std::vector<double> &out_sample)
    {
        assert(out_sample.size() == M_NUM_SAMPLE);
        // Collect latest signal values
        for (auto &sv : m_signal_available) {
            sv.second.m_last_signal = m_platform_io.sample(sv.second.m_batch_idx);
        }
    }

    // Wait for the remaining cycle time to keep Controller loop cadence at 1 second
    void FixedFrequencyAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > FixedFrequencyAgent::report_header(void) const
    {
        return {};
    }

    // Adds to the per-node section of the report
    std::vector<std::pair<std::string, std::string> > FixedFrequencyAgent::report_host(void) const
    {
        return {};
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > FixedFrequencyAgent::report_region(void) const
    {
        return {};
    }

    // Adds trace columns samples and signals of interest
    std::vector<std::string> FixedFrequencyAgent::trace_names(void) const
    {
        std::vector<std::string> names;
        // Build name in the format: "NVML::FREQUENCY-board-0"
        for (auto &sv : m_signal_available) {
            names.push_back(sv.first + "-" + m_platform_topo.domain_type_to_name(GEOPM_DOMAIN_BOARD) + "-0");
        }
        return names;
    }

    // Updates the trace with values for samples and signals from this Agent
    void FixedFrequencyAgent::trace_values(std::vector<double> &values)
    {
        int values_idx = 0;
        // Default assumption is that every signal added should be in the trace
        for (auto &sv : m_signal_available) {
            values[values_idx] = sv.second.m_last_signal;
            ++values_idx;
        }
    }

    std::vector<std::function<std::string(double)> > FixedFrequencyAgent::trace_formats(void) const
    {
        std::vector<std::function<std::string(double)>> trace_formats;
        for (auto &sv : m_signal_available) {
            trace_formats.push_back(m_platform_io.format_function(sv.first));
        }
        return trace_formats;
    }

    // Name used for registration with the Agent factory
    std::string FixedFrequencyAgent::plugin_name(void)
    {
        return "fixed_frequency";
    }

    // Used by the factory to create objects of this type
    std::unique_ptr<Agent> FixedFrequencyAgent::make_plugin(void)
    {
        return geopm::make_unique<FixedFrequencyAgent>();
    }

    // Describes expected policies to be provided by the resource manager or user
    std::vector<std::string> FixedFrequencyAgent::policy_names(void)
    {
        return {"ACCELERATOR_FREQUENCY",
                "CORE_FREQUENCY",
                "UNCORE_FREQUENCY",
               };
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> FixedFrequencyAgent::sample_names(void)
    {
        return {};
    }
}
