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

#include "NVMLBoardUtilizationAgent.hpp"

#include <iostream>

#include <cmath>
#include <cassert>
#include <algorithm>

#include "PluginFactory.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Agg.hpp"

#include <string>

namespace geopm
{
    NVMLBoardUtilizationAgent::NVMLBoardUtilizationAgent():
        NVMLBoardUtilizationAgent(platform_io(), platform_topo())
    {
    }

    NVMLBoardUtilizationAgent::NVMLBoardUtilizationAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(platform_topo())
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(0.05) // 50mS wait
        , m_do_write_batch(false)
        , m_signal_available({{"NVML::FREQUENCY", {}},
                              {"NVML::UTILIZATION_ACCELERATOR", {}},
                              {"NVML::POWER", {}},
                              {"NVML::TOTAL_ENERGY_CONSUMPTION", {}},
                              {"FREQUENCY", {}}
                             })
        , m_control_available({{"NVML::FREQUENCY_CONTROL", {}},
                               {"FREQUENCY", {}}
                              })
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void NVMLBoardUtilizationAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        if (level == 0) {
            init_platform_io();
        }
    }

    void NVMLBoardUtilizationAgent::init_platform_io(void)
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
    void NVMLBoardUtilizationAgent::validate_policy(std::vector<double> &in_policy) const
    {
        assert(in_policy.size() == M_NUM_POLICY);
    }

    // Distribute incoming policy to children
    void NVMLBoardUtilizationAgent::split_policy(const std::vector<double>& in_policy,
                                    std::vector<std::vector<double> >& out_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool NVMLBoardUtilizationAgent::do_send_policy(void) const
    {
        return true;
    }

    void NVMLBoardUtilizationAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double>& out_sample)
    {

    }

    // Indicate whether to send samples up to the parent
    bool NVMLBoardUtilizationAgent::do_send_sample(void) const
    {
        return false;
    }

    // This controller uses a ganged (treating all of a given device type as a group) approach
    // to avoid the need for tracking the mapping of individual CPUs to Accelerators.
    //
    // Basic approach:
    //  - If all GPUs are below or equal to threshold 0, set all GPU and CPU frequencies
    //    to corresponding sub_thresh_0 values
    //  - If all GPUs are below or equal to threshold 1, set all GPU and CPU frequencies
    //    to corresponding sub_thresh_1 values
    //  - If all GPUs are above threshold 1, set all GPU and CPU frequencies
    //    to corresponding above_thresh_1 values
    void NVMLBoardUtilizationAgent::adjust_platform(const std::vector<double>& in_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);
        m_do_write_batch = false;

        double utilization_accelerator = m_signal_available.at("NVML::UTILIZATION_ACCELERATOR").m_last_signal;
        double accel_freq_request = NAN;
        double xeon_freq_request = NAN;

        if (!std::isnan(utilization_accelerator)) {
            if(utilization_accelerator <= in_policy[M_POLICY_ACCELERATOR_UTIL_THRESH_0]) {
                accel_freq_request = in_policy[M_POLICY_ACCELERATOR_FREQ_SUB_THRESH_0];
                xeon_freq_request = in_policy[M_POLICY_XEON_FREQ_SUB_THRESH_0];
            } else if(utilization_accelerator <= in_policy[M_POLICY_ACCELERATOR_UTIL_THRESH_1]) {
                accel_freq_request = in_policy[M_POLICY_ACCELERATOR_FREQ_SUB_THRESH_1];
                xeon_freq_request = in_policy[M_POLICY_XEON_FREQ_SUB_THRESH_1];
            } else {
                accel_freq_request = in_policy[M_POLICY_ACCELERATOR_FREQ_ABOVE_THRESH_1];
                xeon_freq_request = in_policy[M_POLICY_XEON_FREQ_ABOVE_THRESH_1];
            }
        }

        if (!std::isnan(accel_freq_request) && !std::isnan(xeon_freq_request)) {
            if (accel_freq_request != m_signal_available.at("NVML::FREQUENCY").m_last_signal ||
                xeon_freq_request != m_signal_available.at("FREQUENCY").m_last_signal) {
                // set NVML frequency control
                auto freq_ctl_itr = m_control_available.find("NVML::FREQUENCY_CONTROL");
                if (accel_freq_request != freq_ctl_itr->second.m_last_setting) {
                    m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, accel_freq_request);
                    freq_ctl_itr->second.m_last_setting = accel_freq_request;
                    ++m_accelerator_frequency_requests;
                }

                // set Xeon frequency control
                freq_ctl_itr = m_control_available.find("FREQUENCY");
                if (xeon_freq_request != freq_ctl_itr->second.m_last_setting) {
                    m_platform_io.adjust(freq_ctl_itr->second.m_batch_idx, xeon_freq_request);
                    freq_ctl_itr->second.m_last_setting = xeon_freq_request;
                }

                m_do_write_batch = true;
            }
        }
    }

    //If new values have been adjusted, write
    bool NVMLBoardUtilizationAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    // Read signals from the platform and calculate samples to be sent up
    void NVMLBoardUtilizationAgent::sample_platform(std::vector<double> &out_sample)
    {
        assert(out_sample.size() == M_NUM_SAMPLE);
        // Collect latest signal values
        for (auto &sv : m_signal_available) {
            sv.second.m_last_signal = m_platform_io.sample(sv.second.m_batch_idx);
        }
    }

    // Wait for the remaining cycle time to keep Controller loop cadence at 1 second
    void NVMLBoardUtilizationAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > NVMLBoardUtilizationAgent::report_header(void) const
    {
        return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
    }

    // Adds to the per-node section of the report
    std::vector<std::pair<std::string, std::string> > NVMLBoardUtilizationAgent::report_host(void) const
    {
        return {
            {"Accelerator Frequency Requests", std::to_string(m_accelerator_frequency_requests)},
        };
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > NVMLBoardUtilizationAgent::report_region(void) const
    {
        return {};
    }

    // Adds trace columns samples and signals of interest
    std::vector<std::string> NVMLBoardUtilizationAgent::trace_names(void) const
    {
        std::vector<std::string> names;
        // Build name in the format: "NVML::FREQUENCY-board-0"
        for (auto &sv : m_signal_available) {
            names.push_back(sv.first + "-" + m_platform_topo.domain_type_to_name(GEOPM_DOMAIN_BOARD) + "-0");
        }
        return names;
    }

    // Updates the trace with values for samples and signals from this Agent
    void NVMLBoardUtilizationAgent::trace_values(std::vector<double> &values)
    {
        int values_idx = 0;
        // Default assumption is that every signal added should be in the trace
        for (auto &sv : m_signal_available) {
            values[values_idx] = sv.second.m_last_signal;
            ++values_idx;
        }
    }

    std::vector<std::function<std::string(double)> > NVMLBoardUtilizationAgent::trace_formats(void) const
    {
        std::vector<std::function<std::string(double)>> trace_formats;
        for (auto &sv : m_signal_available) {
            trace_formats.push_back(m_platform_io.format_function(sv.first));
        }
        return trace_formats;
    }

    // Name used for registration with the Agent factory
    std::string NVMLBoardUtilizationAgent::plugin_name(void)
    {
        return "nvml_board_utilization";
    }

    // Used by the factory to create objects of this type
    std::unique_ptr<Agent> NVMLBoardUtilizationAgent::make_plugin(void)
    {
        return geopm::make_unique<NVMLBoardUtilizationAgent>();
    }

    // Describes expected policies to be provided by the resource manager or user
    std::vector<std::string> NVMLBoardUtilizationAgent::policy_names(void)
    {
        return {"ACCELERATOR_UTIL_THRESH_0",
                "ACCELERATOR_FREQUENCY_SUB_THRESH_0",
                "XEON_FREQUENCY_SUB_THRESH_0",
                "ACCELERATOR_UTIL_THRESH_1",
                "ACCELERATOR_FREQUENCY_SUB_THRESH_1",
                "XEON_FREQUENCY_SUB_THRESH_1",
                "ACCELERATOR_FREQUENCY_ABOVE_THRESH_1",
                "XEON_FREQUENCY_ABOVE_THRESH_1"};
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> NVMLBoardUtilizationAgent::sample_names(void)
    {
        return {};
    }
}
