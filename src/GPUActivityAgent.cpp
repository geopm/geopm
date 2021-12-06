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

#include "GPUActivityAgent.hpp"

#include <cmath>
#include <cassert>
#include <algorithm>

#include "PlatformIOProf.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

#include <string>

#include <iostream>

namespace geopm
{

    GPUActivityAgent::GPUActivityAgent()
        : GPUActivityAgent(PlatformIOProf::platform_io(), platform_topo())
    {

    }

    GPUActivityAgent::GPUActivityAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(0.020) // 20ms Wait
        , M_POLICY_PHI_DEFAULT(0.5)
        , M_GPU_ACTIVITY_CUTOFF(0.05)
        , m_do_write_batch(false)
        // This agent approach is meant to allow for quick prototyping through simplifying
        // signal & control addition and usage.  Most changes to signals and controls
        // should be accomplishable with changes to the declaration below (instead of updating
        // init_platform_io, sample_platform, etc).  Signal & control usage is still
        // handled in adjust_platform per usual.
        , m_signal_available({
                              {"FREQUENCY_ACCELERATOR", {
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  true,
                                  {}
                                  }},
                              {"ACCELERATOR_COMPUTE_ACTIVITY", {
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  true,
                                  {}
                                  }},
                              {"UTILIZATION_ACCELERATOR", {
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  true,
                                  {}
                                  }},
                              {"ENERGY_ACCELERATOR", {
                                  GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                  true,
                                  {}
                                  }},
                              {"TIME", {
                                  GEOPM_DOMAIN_BOARD,
                                  false,
                                  {}
                                  }},
                             })
        , m_control_available({
                               {"FREQUENCY_ACCELERATOR_CONTROL", {
                                    GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                    false,
                                    {}
                                    }},
                              })
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void GPUActivityAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_accelerator_frequency_requests = 0;
        m_f_max_resolved = 0;
        m_f_efficient_resolved = 0;
        m_f_range_resolved = 0;
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR); ++domain_idx) {
            m_accelerator_active_region_start.push_back(0.0);
            m_accelerator_active_region_stop.push_back(0.0);
            m_accelerator_active_energy_start.push_back(0.0);
            m_accelerator_active_energy_stop.push_back(0.0);
        }

        if (level == 0) {
            init_platform_io();
        }
    }

    void GPUActivityAgent::init_platform_io(void)
    {
        // populate signals for each domain with batch idx info, default values, etc
        for (auto &sv : m_signal_available) {
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(sv.second.domain); ++domain_idx) {
                signal sgnl = signal{m_platform_io.push_signal(sv.first,
                                                               sv.second.domain,
                                                               domain_idx), NAN, NAN};
                sv.second.signals.push_back(sgnl);
            }
        }

        // populate controls for each domain
        for (auto &sv : m_control_available) {
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(sv.second.domain); ++domain_idx) {
                control ctrl = control{m_platform_io.push_control(sv.first,
                                                                  sv.second.domain,
                                                                  domain_idx), NAN};
                sv.second.controls.push_back(ctrl);
            }
        }

        auto all_names = m_platform_io.control_names();
        if (all_names.find("DCGM::FIELD_UPDATE_RATE") != all_names.end()) {
            // DCGM documentation indicates that users should query no faster than 100ms
            // even though the interface allows for setting the polling rate in the us range.
            // In practice reducing below the 100ms value has proven functional, but should only
            // be attempted if there is a strong need to catch short phase behavior.
            m_platform_io.write_control("DCGM::FIELD_UPDATE_RATE", GEOPM_DOMAIN_BOARD, 0, 0.1); //100ms
            //m_platform_io.write_control("DCGM::FIELD_UPDATE_RATE", GEOPM_DOMAIN_BOARD, 0, 0.001); //1ms
            m_platform_io.write_control("DCGM::MAX_STORAGE_TIME", GEOPM_DOMAIN_BOARD, 0, 1);
            m_platform_io.write_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, 0, 100);
        }
    }

    // Validate incoming policy and configure default policy requests.
    void GPUActivityAgent::validate_policy(std::vector<double> &in_policy) const
    {
        assert(in_policy.size() == M_NUM_POLICY);
        double accel_min_freq = m_platform_io.read_signal("FREQUENCY_MIN_ACCELERATOR", GEOPM_DOMAIN_BOARD, 0);
        double accel_max_freq = m_platform_io.read_signal("FREQUENCY_MAX_ACCELERATOR", GEOPM_DOMAIN_BOARD, 0);

        // Check for NAN to set default values for policy
        if (std::isnan(in_policy[M_POLICY_ACCELERATOR_FREQ_MAX])) {
            in_policy[M_POLICY_ACCELERATOR_FREQ_MAX] = accel_max_freq;
        }

        if (in_policy[M_POLICY_ACCELERATOR_FREQ_MAX] > accel_max_freq ||
            in_policy[M_POLICY_ACCELERATOR_FREQ_MAX] < accel_min_freq ) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): ACCELERATOR_FREQ_MAX out of range: " +
                            std::to_string(in_policy[M_POLICY_ACCELERATOR_FREQ_MAX]) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Not all accelerators provide an 'efficient' frequency signal, and the
        // value provided by the policy may not be valid.  In this case approximating
        // f_efficient as midway between F_min and F_max is reasonable.
        if (std::isnan(in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT])) {
            in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT] = (in_policy[M_POLICY_ACCELERATOR_FREQ_MAX]
                                                             + accel_min_freq) / 2;
        }

        if (in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT] > accel_max_freq ||
            in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT] < accel_min_freq ) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): ACCELERATOR_FREQ_EFFICIENT out of range: " +
                            std::to_string(in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT]) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT] > in_policy[M_POLICY_ACCELERATOR_FREQ_MAX]) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): ACCELERATOR_FREQ_EFFICIENT (" +
                            std::to_string(in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT]) +
                            ") value exceeds ACCELERATOR_FREQ_MAX (" +
                            std::to_string(in_policy[M_POLICY_ACCELERATOR_FREQ_MAX]) +
                            ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // If no phi value is provided assume the default behavior.
        if (std::isnan(in_policy[M_POLICY_ACCELERATOR_PHI])) {
            in_policy[M_POLICY_ACCELERATOR_PHI] = M_POLICY_PHI_DEFAULT;
        }

        if (in_policy[M_POLICY_ACCELERATOR_PHI] < 0.0 ||
            in_policy[M_POLICY_ACCELERATOR_PHI] > 1.0) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): POLICY_ACCELERATOR_PHI value out of range: " +
                            std::to_string(in_policy[M_POLICY_ACCELERATOR_PHI]) + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    // Distribute incoming policy to children
    void GPUActivityAgent::split_policy(const std::vector<double>& in_policy,
                                    std::vector<std::vector<double> >& out_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool GPUActivityAgent::do_send_policy(void) const
    {
        return true;
    }

    void GPUActivityAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double>& out_sample)
    {

    }

    // Indicate whether to send samples up to the parent
    bool GPUActivityAgent::do_send_sample(void) const
    {
        return false;
    }

    void GPUActivityAgent::adjust_platform(const std::vector<double>& in_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);

        m_do_write_batch = false;

        // Primary signal used for frequency recommendation
        auto gpu_active_itr = m_signal_available.find("ACCELERATOR_COMPUTE_ACTIVITY");
        auto gpu_utilization_itr = m_signal_available.find("UTILIZATION_ACCELERATOR");

        // Track energy in the active and passive case for reporting
        auto energy_itr = m_signal_available.find("ENERGY_ACCELERATOR");

        // Per GPU freq
        std::vector<double> board_accelerator_freq_request;

        // Policy provided initial values
        double f_max = in_policy[M_POLICY_ACCELERATOR_FREQ_MAX];
        double f_efficient = in_policy[M_POLICY_ACCELERATOR_FREQ_EFFICIENT];
        double phi = in_policy[M_POLICY_ACCELERATOR_PHI];

        // initial range is needed to apply phi
        double f_range = f_max - f_efficient;

        if (phi > 0.5) {
            //Energy Biased.  Scale F_max down to F_efficient based upon phi value
            //Active region phi usage
            f_max = std::max(f_efficient, f_max - f_range * (phi-0.5) / 0.5);
        }
        else if (phi < 0.5) {
            //Perf Biased.  Scale F_efficient up to F_max based upon phi value
            //Active region phi usage
            f_efficient = std::min(f_max, f_efficient + f_range * (0.5-phi) / 0.5);
        }

        // Recalculate range after phi has been applied
        f_range = f_max - f_efficient;

        // Tracking phi resolved frequencies for the report
        m_f_max_resolved = f_max;
        m_f_efficient_resolved = f_efficient;
        m_f_range_resolved = f_range;

        // Per GPU Frequency Selection
        for (int domain_idx = 0; domain_idx < (int) gpu_active_itr->second.signals.size(); ++domain_idx) {
            // Accelerator Comppute Activity
            double accelerator_compute_activity = gpu_active_itr->second.signals.at(domain_idx).m_last_signal;
            double accelerator_utilization = gpu_utilization_itr->second.signals.at(domain_idx).m_last_signal;

            // Default to F_max
            double f_request = f_max;

            if (!std::isnan(accelerator_compute_activity)) {
                accelerator_compute_activity = std::min(accelerator_compute_activity, 1.0);

                // Frequency selection is based upon the accelerator compute activity.
                // For active regions this means that we scale with the amount of work
                // being done (such as SM_ACTIVE for NVIDIA GPUs).
                //
                // The compute activity is scaled by the GPU Utilization, to help
                // address the issues that come from workloads have short phases that are
                // frequency sensitive.  If a workload has a compute activity of 0.5, and
                // is resident on the GPU for 50% of cycles (0.5) it is treated as having
                // a 1.0 compute activity value
                //
                // For inactive regions the frequency selection is simply the efficient
                // frequency from system characterization.
                //
                // This approach assumes the efficient frequency is suitable as both a
                // baseline for active regions and and inactive regions. This is generally
                // true of the efficient frequency is low power enough at idle due to clock
                // gating or other hardware PM techniques.
                //
                // If f_efficient does not meet these criteria this behavior can still be
                // achieved through tracking the GPU Utilization signal and setting frequency
                // to a separate idle value (f_idle) during regions where GPU Utilizaiton is
                // zero (or below some bar).
                if (!std::isnan(accelerator_utilization) &&
                    accelerator_utilization > 0) {
                    accelerator_utilization = std::min(accelerator_utilization, 1.0);
                    f_request = f_efficient + f_range * (accelerator_compute_activity / accelerator_utilization);
                }
                else {
                    f_request = f_efficient + f_range * accelerator_compute_activity;
                }

                auto time_itr = m_signal_available.find("TIME");
                double time = time_itr->second.signals.at(0).m_last_sample;
                // Tracking logic.  This is not needed for any performance reason,
                // but does provide useful metrics for tracking agent behavior
                if (accelerator_compute_activity >= M_GPU_ACTIVITY_CUTOFF) {
                    m_accelerator_active_region_stop.at(domain_idx) = 0;
                    if (m_accelerator_active_region_start.at(domain_idx) == 0) {
                        m_accelerator_active_region_start.at(domain_idx) = time;
                        m_accelerator_active_energy_start.at(domain_idx) = energy_itr->second.signals.at(domain_idx).m_last_signal;
                    }
                }
                else {
                    if (m_accelerator_active_region_stop.at(domain_idx) == 0) {
                        m_accelerator_active_region_stop.at(domain_idx) = time;
                        m_accelerator_active_energy_stop.at(domain_idx) = energy_itr->second.signals.at(domain_idx).m_last_signal;
                    }
                }
            }

            // Frequency bound checking
            f_request = std::min(f_request, f_max);
            f_request = std::max(f_request, f_efficient);

            // Store frequency request
            board_accelerator_freq_request.push_back(f_request);
        }

        if (!board_accelerator_freq_request.empty()) {
            // set frequency control per accelerator
            auto freq_ctl_itr = m_control_available.find("FREQUENCY_ACCELERATOR_CONTROL");
            for (int domain_idx = 0; domain_idx < (int) freq_ctl_itr->second.controls.size(); ++domain_idx) {
                if (board_accelerator_freq_request.at(domain_idx) !=
                    freq_ctl_itr->second.controls.at(domain_idx).m_last_setting) {
                    m_platform_io.adjust(freq_ctl_itr->second.controls.at(domain_idx).m_batch_idx,
                                         board_accelerator_freq_request.at(domain_idx));
                    freq_ctl_itr->second.controls.at(domain_idx).m_last_setting =
                                         board_accelerator_freq_request.at(domain_idx);
                    ++m_accelerator_frequency_requests;
                }
            }
            m_do_write_batch = true;
        }
    }

    // If controls have a valid updated value write them.
    bool GPUActivityAgent::do_write_batch(void) const
    {
        return m_do_write_batch;
    }

    // Read signals from the platform and calculate samples to be sent up
    void GPUActivityAgent::sample_platform(std::vector<double> &out_sample)
    {
        assert(out_sample.size() == M_NUM_SAMPLE);

        // Collect latest signal values
        for (auto &sv : m_signal_available) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                double curr_value = m_platform_io.sample(sv.second.signals.at(domain_idx).m_batch_idx);

                if (sv.first == "ENERGY_ACCELERATOR") {
                    sv.second.signals.at(domain_idx).m_last_sample = curr_value -
                                                                     sv.second.signals.at(domain_idx).m_last_signal;
                }
                else {
                    sv.second.signals.at(domain_idx).m_last_sample = sv.second.signals.at(domain_idx).m_last_signal;
                }
                sv.second.signals.at(domain_idx).m_last_signal = curr_value;
            }
        }
    }

    // Wait for the remaining cycle time to keep Controller loop cadence
    void GPUActivityAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > GPUActivityAgent::report_header(void) const
    {
        return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
    }

    // Adds number of frquency requests to the per-node section of the report
    std::vector<std::pair<std::string, std::string> > GPUActivityAgent::report_host(void) const
    {
        std::vector<std::pair<std::string, std::string> > result;

        result.push_back({"Accelerator Frequency Requests", std::to_string(m_accelerator_frequency_requests)});
        result.push_back({"Resolved Max Frequency", std::to_string(m_f_max_resolved)});
        result.push_back({"Resolved Efficient Frequency", std::to_string(m_f_efficient_resolved)});
        result.push_back({"Resolved Frequency Range", std::to_string(m_f_range_resolved)});

        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR); ++domain_idx) {
            double energy_stop = m_accelerator_active_energy_stop.at(domain_idx);
            double energy_start = m_accelerator_active_energy_start.at(domain_idx);
            double region_stop = m_accelerator_active_region_stop.at(domain_idx);
            double region_start =  m_accelerator_active_region_start.at(domain_idx);
            result.push_back({"Accelerator " + std::to_string(domain_idx) +
                              " Active Region Energy", std::to_string(energy_stop - energy_start)});
            result.push_back({"Accelerator " + std::to_string(domain_idx) +
                              " Active Region Time", std::to_string(region_stop - region_start)});
            result.push_back({"Accelerator " + std::to_string(domain_idx) +
                              " Active Region Start Time", std::to_string(region_start)});
            result.push_back({"Accelerator " + std::to_string(domain_idx) +
                              " Active Region Stop Time", std::to_string(region_stop)});
        }

        return result;
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > GPUActivityAgent::report_region(void) const
    {
        return {};
    }

    // Adds trace columns samples and signals of interest
    std::vector<std::string> GPUActivityAgent::trace_names(void) const
    {
        std::vector<std::string> names;

        // Signals
        // Automatically build name in the format: "FREQUENCY_ACCELERATOR-board_accelerator-0"
        for (auto &sv : m_signal_available) {
            if (sv.second.trace_signal) {
                for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                    names.push_back(sv.first + "-" + m_platform_topo.domain_type_to_name(sv.second.domain) + "-" + std::to_string(domain_idx));
                }
            }
        }
        // Controls
        // Automatically build name in the format: "FREQUENCY_ACCELERATOR_CONTROL-board_accelerator-0"
        for (auto &sv : m_control_available) {
            if (sv.second.trace_control) {
                for (int domain_idx = 0; domain_idx < (int) sv.second.controls.size(); ++domain_idx) {
                    names.push_back(sv.first + "-" + m_platform_topo.domain_type_to_name(sv.second.domain) + "-" + std::to_string(domain_idx));
                }
            }
        }

        return names;

    }

    // Updates the trace with values for samples and signals from this Agent
    void GPUActivityAgent::trace_values(std::vector<double> &values)
    {
        int values_idx = 0;

        //default assumption is that every signal added should be in the trace
        for (auto &sv : m_signal_available) {
            if (sv.second.trace_signal) {
                for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                    values[values_idx] = sv.second.signals.at(domain_idx).m_last_signal;
                    ++values_idx;
                }
            }
        }

        for (auto &sv : m_control_available) {
            if (sv.second.trace_control) {
                for (int domain_idx = 0; domain_idx < (int) sv.second.controls.size(); ++domain_idx) {
                    values[values_idx] = sv.second.controls.at(domain_idx).m_last_setting;
                    ++values_idx;
                }
            }
        }
    }

    std::vector<std::function<std::string(double)> > GPUActivityAgent::trace_formats(void) const
    {
        std::vector<std::function<std::string(double)>> trace_formats;
        for (auto &sv : m_signal_available) {
            if (sv.second.trace_signal) {
                for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                    trace_formats.push_back(m_platform_io.format_function(sv.first));
                }
            }
        }

        for (auto &sv : m_control_available) {
            if (sv.second.trace_control) {
                for (int domain_idx = 0; domain_idx < (int) sv.second.controls.size(); ++domain_idx) {
                    trace_formats.push_back(m_platform_io.format_function(sv.first));
                }
            }
        }

        return trace_formats;
    }

    // Name used for registration with the Agent factory
    std::string GPUActivityAgent::plugin_name(void)
    {
        return "gpu_activity";
    }

    // Used by the factory to create objects of this type
    std::unique_ptr<Agent> GPUActivityAgent::make_plugin(void)
    {
        return geopm::make_unique<GPUActivityAgent>();
    }

    // Describes expected policies to be provided by the resource manager or user
    std::vector<std::string> GPUActivityAgent::policy_names(void)
    {
        return {"ACCELERATOR_FREQ_MAX", "ACCELERATOR_FREQ_EFFICIENT", "ACCELERATOR_PHI"};
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> GPUActivityAgent::sample_names(void)
    {
        return {};
    }
}
