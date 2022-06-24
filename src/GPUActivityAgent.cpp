/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "GPUActivityAgent.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

#include "geopm/Agg.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"

#include "PlatformIOProf.hpp"

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
        , M_WAIT_SEC(0.020) // 20ms wait default
        , M_POLICY_PHI_DEFAULT(0.5)
        , M_GPU_ACTIVITY_CUTOFF(0.05)
        , M_NUM_GPU(m_platform_topo.num_domain(GEOPM_DOMAIN_GPU))
        , m_do_write_batch(false)
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void GPUActivityAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_gpu_frequency_requests = 0;
        m_gpu_frequency_clipped = 0;
        m_f_max = 0;
        m_f_efficient = 0;
        m_f_range = 0;

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_active_region_start.push_back(0.0);
            m_gpu_active_region_stop.push_back(0.0);
            m_gpu_active_energy_start.push_back(0.0);
            m_gpu_active_energy_stop.push_back(0.0);
        }

        if (level == 0) {
            init_platform_io();
        }
    }

    void GPUActivityAgent::init_platform_io(void)
    {

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_freq_status.push_back({m_platform_io.push_signal("GPU_CORE_FREQUENCY_STATUS",
                                         GEOPM_DOMAIN_GPU,
                                         domain_idx), NAN});
            m_gpu_core_activity.push_back({m_platform_io.push_signal("GPU_CORE_ACTIVITY",
                                              GEOPM_DOMAIN_GPU,
                                              domain_idx), NAN});
            m_gpu_utilization.push_back({m_platform_io.push_signal("GPU_UTILIZATION",
                                         GEOPM_DOMAIN_GPU,
                                         domain_idx), NAN});
            m_gpu_energy.push_back({m_platform_io.push_signal("GPU_ENERGY",
                                    GEOPM_DOMAIN_GPU,
                                    domain_idx), NAN});
        }

        m_time = {m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0), NAN};

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_freq_control.push_back(control{m_platform_io.push_control("GPU_CORE_FREQUENCY_CONTROL",
                                         GEOPM_DOMAIN_GPU,
                                         domain_idx), NAN});
        }

        auto all_names = m_platform_io.control_names();
        if (all_names.find("DCGM::FIELD_UPDATE_RATE") != all_names.end()) {
            // DCGM documentation indicates that users should query no faster than 100ms
            // even though the interface allows for setting the polling rate in the us range.
            // In practice reducing below the 100ms value has proven functional, but should only
            // be attempted if there is a proven need to catch short phase behavior that cannot
            // be accomplished with the default settings.
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
        double gpu_min_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
        double gpu_max_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

        // Check for NAN to set default values for policy
        if (std::isnan(in_policy[M_POLICY_GPU_FREQ_MAX])) {
            in_policy[M_POLICY_GPU_FREQ_MAX] = gpu_max_freq;
        }

        if (in_policy[M_POLICY_GPU_FREQ_MAX] > gpu_max_freq ||
            in_policy[M_POLICY_GPU_FREQ_MAX] < gpu_min_freq ) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): GPU_FREQ_MAX out of range: " +
                            std::to_string(in_policy[M_POLICY_GPU_FREQ_MAX]) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Not all gpus provide an 'efficient' frequency signal, and the
        // value provided by the policy may not be valid.  In this case approximating
        // f_efficient as midway between F_min and F_max is reasonable.
        if (std::isnan(in_policy[M_POLICY_GPU_FREQ_EFFICIENT])) {
            in_policy[M_POLICY_GPU_FREQ_EFFICIENT] = (in_policy[M_POLICY_GPU_FREQ_MAX]
                                                      + gpu_min_freq) / 2;
        }

        if (in_policy[M_POLICY_GPU_FREQ_EFFICIENT] > gpu_max_freq ||
            in_policy[M_POLICY_GPU_FREQ_EFFICIENT] < gpu_min_freq ) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): GPU_FREQ_EFFICIENT out of range: " +
                            std::to_string(in_policy[M_POLICY_GPU_FREQ_EFFICIENT]) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (in_policy[M_POLICY_GPU_FREQ_EFFICIENT] > in_policy[M_POLICY_GPU_FREQ_MAX]) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): GPU_FREQ_EFFICIENT (" +
                            std::to_string(in_policy[M_POLICY_GPU_FREQ_EFFICIENT]) +
                            ") value exceeds GPU_FREQ_MAX (" +
                            std::to_string(in_policy[M_POLICY_GPU_FREQ_MAX]) +
                            ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // If no phi value is provided assume the default behavior.
        if (std::isnan(in_policy[M_POLICY_GPU_PHI])) {
            in_policy[M_POLICY_GPU_PHI] = M_POLICY_PHI_DEFAULT;
        }

        if (in_policy[M_POLICY_GPU_PHI] < 0.0 ||
            in_policy[M_POLICY_GPU_PHI] > 1.0) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): POLICY_GPU_PHI value out of range: " +
                            std::to_string(in_policy[M_POLICY_GPU_PHI]) + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // If no sample period is provided assume the default behavior
        if (std::isnan(in_policy[M_POLICY_SAMPLE_PERIOD])) {
            in_policy[M_POLICY_SAMPLE_PERIOD] = M_WAIT_SEC;
        }

        if (!std::isnan(in_policy[M_POLICY_SAMPLE_PERIOD])) {
            if (in_policy[M_POLICY_SAMPLE_PERIOD] <= 0.0) {
                throw Exception("GPUActivityAgent::" + std::string(__func__) +
                                "(): Sample period must be greater than 0.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);

            }
        }

        // Policy provided initial values
        double f_max = in_policy[M_POLICY_GPU_FREQ_MAX];
        double f_efficient = in_policy[M_POLICY_GPU_FREQ_EFFICIENT];
        double phi = in_policy[M_POLICY_GPU_PHI];

        // initial range is needed to apply phi
        double f_range = f_max - f_efficient;

        // If phi is not 0.5 we move into the energy or performance biased regions
        if (phi > 0.5) {
            // Energy Biased.  Scale F_max down to F_efficient based upon phi value
            // Active region phi usage
            f_max = std::max(f_efficient, f_max - f_range * (phi-0.5) / 0.5);
        }
        else if (phi < 0.5) {
            // Perf Biased.  Scale F_efficient up to F_max based upon phi value
            // Active region phi usage
            f_efficient = std::min(f_max, f_efficient + f_range * (0.5-phi) / 0.5);
        }

        //Update Policy
        in_policy[M_POLICY_GPU_FREQ_MAX] = f_max;
        in_policy[M_POLICY_GPU_FREQ_EFFICIENT] = f_efficient;
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

        // Update wait time based on policy
        if (in_policy[M_POLICY_SAMPLE_PERIOD] != M_WAIT_SEC)  {
            M_WAIT_SEC = in_policy[M_POLICY_SAMPLE_PERIOD];
        }

        m_do_write_batch = false;

        // Per GPU freq
        std::vector<double> gpu_freq_request;

        // Values after phi has been applied
        m_f_max = in_policy[M_POLICY_GPU_FREQ_MAX];
        m_f_efficient = in_policy[M_POLICY_GPU_FREQ_EFFICIENT];
        m_f_range = m_f_max - m_f_efficient;

        // Per GPU Frequency Selection
        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            // gpu Compute Activity - Primary signal used for frequency recommendation
            double gpu_core_activity = m_gpu_core_activity.at(domain_idx).value;
            // gpu Utilization - Used to scale activity for short GPU phases
            double gpu_utilization = m_gpu_utilization.at(domain_idx).value;

            // Default to F_max
            double f_request = m_f_max;

            if (!std::isnan(gpu_core_activity)) {
                // Boundary Checking
                gpu_core_activity = std::min(gpu_core_activity, 1.0);

                // Frequency selection is based upon the gpu compute activity.
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
                // true when the efficient frequency consumes low power at idle due to clock
                // gating or other hardware PM techniques.
                //
                // If f_efficient does not meet these criteria this behavior can still be
                // achieved through tracking the GPU Utilization signal and setting frequency
                // to a separate idle value (f_idle) during regions where GPU Utilizaiton is
                // zero (or below some bar).
                if (!std::isnan(gpu_utilization) &&
                    gpu_utilization > 0) {
                    gpu_utilization = std::min(gpu_utilization, 1.0);
                    f_request = m_f_efficient + m_f_range * (gpu_core_activity / gpu_utilization);
                }
                else {
                    f_request = m_f_efficient + m_f_range * gpu_core_activity;
                }

                // Tracking logic.  This is not needed for any performance reason,
                // but does provide useful metrics for tracking agent behavior
                if (gpu_core_activity >= M_GPU_ACTIVITY_CUTOFF) {
                    m_gpu_active_region_stop.at(domain_idx) = 0;
                    if (m_gpu_active_region_start.at(domain_idx) == 0) {
                        m_gpu_active_region_start.at(domain_idx) = m_time.value;
                        m_gpu_active_energy_start.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
                    }
                }
                else {
                    if (m_gpu_active_region_stop.at(domain_idx) == 0) {
                        m_gpu_active_region_stop.at(domain_idx) = m_time.value;
                        m_gpu_active_energy_stop.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
                    }
                }
            }

            // Frequency clamping
            if (f_request > m_f_max || f_request < m_f_efficient) {
                ++m_gpu_frequency_clipped;
            }
            f_request = std::min(f_request, m_f_max);
            f_request = std::max(f_request, m_f_efficient);

            // Store frequency request
            gpu_freq_request.push_back(f_request);
        }

        if (!gpu_freq_request.empty()) {
            // set frequency control per gpu
            for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
                if (gpu_freq_request.at(domain_idx) !=
                    m_gpu_freq_control.at(domain_idx).last_setting) {
                    m_platform_io.adjust(m_gpu_freq_control.at(domain_idx).batch_idx,
                                         gpu_freq_request.at(domain_idx));
                    m_gpu_freq_control.at(domain_idx).last_setting =
                                         gpu_freq_request.at(domain_idx);
                    ++m_gpu_frequency_requests;
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
        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_freq_status.at(domain_idx).value = m_platform_io.sample(m_gpu_freq_status.at(
                                                                          domain_idx).batch_idx);
            m_gpu_core_activity.at(domain_idx).value = m_platform_io.sample(m_gpu_core_activity.at(
                                                                               domain_idx).batch_idx);
            m_gpu_utilization.at(domain_idx).value = m_platform_io.sample(m_gpu_utilization.at(
                                                                          domain_idx).batch_idx);

            m_gpu_energy.at(domain_idx).value = m_platform_io.sample(m_gpu_energy.at(
                                                                     domain_idx).batch_idx);
        }

        m_time.value = m_platform_io.sample(m_time.batch_idx);
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

        result.push_back({"GPU Frequency Requests", std::to_string(m_gpu_frequency_requests)});
        result.push_back({"GPU Clipped Frequency Requests", std::to_string(m_gpu_frequency_clipped)});
        result.push_back({"Resolved Max Frequency", std::to_string(m_f_max)});
        result.push_back({"Resolved Efficient Frequency", std::to_string(m_f_efficient)});
        result.push_back({"Resolved Frequency Range", std::to_string(m_f_range)});

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            double energy_stop = m_gpu_active_energy_stop.at(domain_idx);
            double energy_start = m_gpu_active_energy_start.at(domain_idx);
            double region_stop = m_gpu_active_region_stop.at(domain_idx);
            double region_start =  m_gpu_active_region_start.at(domain_idx);
            result.push_back({"GPU " + std::to_string(domain_idx) +
                              " Active Region Energy", std::to_string(energy_stop - energy_start)});
            result.push_back({"GPU " + std::to_string(domain_idx) +
                              " Active Region Time", std::to_string(region_stop - region_start)});
        }

        return result;
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > GPUActivityAgent::report_region(void) const
    {
        return {};
    }

    // Adds trace columns signals of interest
    std::vector<std::string> GPUActivityAgent::trace_names(void) const
    {
        return {};
    }

    // Updates the trace with values for signals from this Agent
    void GPUActivityAgent::trace_values(std::vector<double> &values)
    {
    }

    std::vector<std::function<std::string(double)> > GPUActivityAgent::trace_formats(void) const
    {
        return {};
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
        return {"GPU_FREQ_MAX", "GPU_FREQ_EFFICIENT", "GPU_PHI", "SAMPLE_PERIOD"};
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> GPUActivityAgent::sample_names(void)
    {
        return {};
    }
}
