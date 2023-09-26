/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "GPUActivityAgent.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include "geopm/Agg.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm_debug.hpp"

#include "PlatformIOProf.hpp"
#include "Waiter.hpp"
#include "Environment.hpp"

// IDLE SAMPLE COUNT of 10 is based upon a study of the idle behavior of CORAL-2
// workloads of interest assuming the default 20ms sample rate (200ms idle).
// We could use 200ms as the default for the agent, but this does not provide a
// mechanism for user control of the idle period.  Using a count provides partial
// user control in that the idleness period is defined by the requested agent
// control loop time.
#define IDLE_SAMPLE_COUNT 10

namespace geopm
{

    GPUActivityAgent::GPUActivityAgent()
        : GPUActivityAgent(PlatformIOProf::platform_io(), platform_topo(),
                           Waiter::make_unique(environment().period(M_WAIT_SEC)))
    {

    }

    GPUActivityAgent::GPUActivityAgent(PlatformIO &plat_io, const PlatformTopo &topo,
                                       std::shared_ptr<Waiter> waiter)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , M_POLICY_PHI_DEFAULT(0.5)
        , M_GPU_ACTIVITY_CUTOFF(0.05)
        , M_NUM_GPU(m_platform_topo.num_domain(
                    GEOPM_DOMAIN_GPU))
        , M_NUM_GPU_CHIP(m_platform_topo.num_domain(
                         GEOPM_DOMAIN_GPU_CHIP))
        , M_NUM_CHIP_PER_GPU(M_NUM_GPU_CHIP / M_NUM_GPU)
        , m_do_write_batch(false)
        , m_do_send_policy(true)
        , m_agent_domain_count(0)
        , m_agent_domain(0)
        , m_gpu_frequency_requests(0.0)
        , m_gpu_frequency_clipped(0.0)
        , m_freq_gpu_min(0.0)
        , m_freq_gpu_max(0.0)
        , m_freq_gpu_efficient(0.0)
        , m_resolved_f_gpu_max(0.0)
        , m_resolved_f_gpu_efficient(0.0)
        , m_f_range(0.0)
        , m_time({})
        , m_waiter(std::move(waiter))
    {

    }

    void GPUActivityAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_gpu_frequency_requests = 0;
        m_gpu_frequency_clipped = 0;
        m_resolved_f_gpu_max = 0;
        m_resolved_f_gpu_efficient = 0;
        m_f_range = 0;

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_active_region_start.push_back(0.0);
            m_gpu_active_region_stop.push_back(0.0);
            m_gpu_active_energy_start.push_back(0.0);
            m_gpu_active_energy_stop.push_back(0.0);

            m_gpu_on_time.push_back(0.0);
            m_gpu_on_energy.push_back(0.0);
            m_prev_gpu_energy.push_back(0.0);
        }

        if (level == 0) {
            init_platform_io();
        }
    }

    // Push signals and controls for future batch read/write
    void GPUActivityAgent::init_platform_io(void)
    {

        std::vector<int> control_domains;
        control_domains.push_back(m_platform_io.control_domain_type("GPU_CORE_FREQUENCY_MIN_CONTROL"));
        control_domains.push_back(m_platform_io.control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"));

        std::vector<int> signal_domains;
        signal_domains.push_back(m_platform_io.signal_domain_type("GPU_CORE_FREQUENCY_STATUS"));
        signal_domains.push_back(m_platform_io.signal_domain_type("GPU_CORE_ACTIVITY"));
        signal_domains.push_back(m_platform_io.signal_domain_type("GPU_UTILIZATION"));

        // We'll use the coarsest granularity supported by any of the controls or signals except Energy
        // i.e. If one control supports domain GPU and another supports domain GPU_CHIP
        m_agent_domain = std::min(*std::min_element(std::begin(control_domains), std::end(control_domains)),
                                  *std::min_element(std::begin(signal_domains), std::end(signal_domains)));

#ifdef GEOPM_DEBUG
        {
            int max_agent_domain = std::max(*std::max_element(std::begin(control_domains), std::end(control_domains)),
                                            *std::max_element(std::begin(signal_domains), std::end(signal_domains)));

            if (m_agent_domain != max_agent_domain) {
                throw Exception("GPUActivityAgent::" + std::string(__func__) +
                                "(): Required signals and controls do not all exist at the same domain.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
#endif

        if (m_agent_domain != GEOPM_DOMAIN_GPU &&
            m_agent_domain != GEOPM_DOMAIN_GPU_CHIP) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): Required signals and controls do not exist at the " +
                            "GPU or GPU_CHIP domain!", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_agent_domain_count = m_platform_topo.num_domain(m_agent_domain);

        for (int domain_idx = 0; domain_idx < m_agent_domain_count; ++domain_idx) {
            // Signals
            m_gpu_core_activity.push_back({m_platform_io.push_signal("GPU_CORE_ACTIVITY",
                                           m_agent_domain,
                                           domain_idx), NAN});
            m_gpu_utilization.push_back({m_platform_io.push_signal("GPU_UTILIZATION",
                                         m_agent_domain,
                                         domain_idx), NAN});

            // Controls
            m_gpu_freq_min_control.push_back(m_control{m_platform_io.push_control("GPU_CORE_FREQUENCY_MIN_CONTROL",
                                                       m_agent_domain,
                                                       domain_idx), NAN});
            m_gpu_freq_max_control.push_back(m_control{m_platform_io.push_control("GPU_CORE_FREQUENCY_MAX_CONTROL",
                                                       m_agent_domain,
                                                       domain_idx), NAN});
            m_gpu_idle_timer.push_back(IDLE_SAMPLE_COUNT);
            m_gpu_idle_samples.push_back(0);
        }

        // We treat energy & time as special cases and only use them at a specific domain.
        // This is because energy & time are used for for tracking agent behavior/reporting
        // and do not impact the agent algorithm
        m_time = {m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0), NAN};

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_energy.push_back({m_platform_io.push_signal("GPU_ENERGY",
                                    m_platform_io.signal_domain_type("GPU_ENERGY"),
                                    domain_idx), NAN});
        }

        m_freq_gpu_min = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
        m_freq_gpu_max = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

        const auto ALL_NAMES = m_platform_io.signal_names();
        // F efficient values
        const std::string FE_CONSTCONFIG = "CONST_CONFIG::GPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY";
        const std::string FE_SIG_NAME = "LEVELZERO::GPU_CORE_FREQUENCY_EFFICIENT";
        if (ALL_NAMES.count(FE_CONSTCONFIG) != 0) {
            m_freq_gpu_efficient = m_platform_io.read_signal(FE_CONSTCONFIG, GEOPM_DOMAIN_BOARD, 0);
        }
        else if (ALL_NAMES.count(FE_SIG_NAME) != 0) {
            m_freq_gpu_efficient = m_platform_io.read_signal(FE_SIG_NAME, GEOPM_DOMAIN_BOARD, 0);
        }
        else {
            m_freq_gpu_efficient = (m_freq_gpu_max + m_freq_gpu_min) / 2;
        }

        if (m_freq_gpu_efficient > m_freq_gpu_max ||
            m_freq_gpu_efficient < m_freq_gpu_min ) {
            throw Exception("GPUActivityAgent::" + std::string(__func__) +
                            "(): GPU efficient frequency out of range: " +
                            std::to_string(m_freq_gpu_efficient) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    // Validate incoming policy and configure default policy requests.
    void GPUActivityAgent::validate_policy(std::vector<double> &in_policy) const
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY,
                           "GPUActivityAgent::validate_policy(): policy vector incorrectly sized");

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
    }

    // Distribute incoming policy to children
    void GPUActivityAgent::split_policy(const std::vector<double>& in_policy,
                                        std::vector<std::vector<double> >& out_policy)
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY,
                           "GPUActivityAgent::split_policy(): policy vector incorrectly sized");
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool GPUActivityAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
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
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY,
                           "GPUActivityAgent::adjust_platform(): policy vector incorrectly sized");

        m_do_send_policy = false;
        m_do_write_batch = false;

        // Per GPU freq
        std::vector<double> gpu_freq_request;
        std::vector<double> gpu_scoped_core_activity;

        double f_gpu_range = m_freq_gpu_max - m_freq_gpu_efficient;
        double phi = in_policy[M_POLICY_GPU_PHI];

        // Default phi = 0.5 case is full fe to fmax range
        // Core
        m_resolved_f_gpu_max = m_freq_gpu_max;
        m_resolved_f_gpu_efficient = m_freq_gpu_efficient;

        // If phi is not 0.5 we move into the energy or performance biased behavior
        if (phi > 0.5) {
            // Energy Biased.  Scale F_max down to F_efficient based upon phi value
            // Active region phi usage
            m_resolved_f_gpu_max = std::max(m_freq_gpu_efficient, m_freq_gpu_max -
                                                                  f_gpu_range *
                                                                  (phi-0.5) / 0.5);
        }
        else if (phi < 0.5) {
            // Perf Biased.  Scale F_efficient up to F_max based upon phi value
            // Active region phi usage
            m_resolved_f_gpu_efficient = std::min(m_freq_gpu_max, m_freq_gpu_efficient +
                                                                  f_gpu_range *
                                                                  (0.5-phi) / 0.5);
        }

        // Values after phi has been applied
        m_f_range = m_resolved_f_gpu_max - m_resolved_f_gpu_efficient;

        // Per GPU Frequency Selection
        for (int domain_idx = 0; domain_idx < m_agent_domain_count; ++domain_idx) {
            // gpu Compute Activity - Primary signal used for frequency recommendation
            double gpu_core_activity = m_gpu_core_activity.at(domain_idx).value;
            // gpu Utilization - Used to scale activity for short GPU phases
            double gpu_utilization = m_gpu_utilization.at(domain_idx).value;

            // Default to F_max
            double f_request = m_resolved_f_gpu_max;

            if (!std::isnan(gpu_core_activity)) {
                // Boundary Checking
                gpu_core_activity = std::min(gpu_core_activity, 1.0);

                // Frequency selection is based upon the gpu compute activity.
                // For active regions this means that we scale with the amount of work
                // being done (such as SM_ACTIVE for NVIDIA GPUs).
                //
                // The compute activity is scaled by the GPU Utilization, to help
                // address the issues that come from workloads that have short phases that are
                // frequency sensitive.  If a workload has a compute activity of 0.5, and
                // is resident on the GPU for 50% of cycles (0.5) it is treated as having
                // a 1.0 compute activity value
                //
                // For inactive regions the frequency selection is simply the efficient
                // frequency from system characterization.
                //
                // This approach assumes the efficient frequency is suitable as both a
                // baseline for active regions and inactive regions. This is generally
                // true when the efficient frequency consumes low power at idle due to clock
                // gating or other hardware PM techniques.
                //
                // If f_efficient does not meet these criteria this behavior can still be
                // achieved through tracking the GPU Utilization signal and setting frequency
                // to a separate idle value (f_idle) during regions where GPU Utilization is
                // zero (or below some bar).
                if (!std::isnan(gpu_utilization) &&
                    gpu_utilization > 0) {
                    gpu_utilization = std::min(gpu_utilization, 1.0);
                    f_request = m_resolved_f_gpu_efficient + m_f_range * (gpu_core_activity / gpu_utilization);
                }
                else {
                    f_request = m_resolved_f_gpu_efficient + m_f_range * gpu_core_activity;
                }

                // We're using the activity of the first
                // GPU_CHIP per GPU as a rough estimate of total GPU activity
                // for tracking the workload region of interest later on.
                // This is non-ideal, but is intended to be a temporary
                // solution to the lack of GPU region support and may be
                // removed when that support is added to GEOPM.
                if (domain_idx % (M_NUM_CHIP_PER_GPU) == 0) {
                    gpu_scoped_core_activity.push_back(gpu_core_activity);
                }
            }

            // Frequency clamping
            if (f_request > m_resolved_f_gpu_max || f_request < m_resolved_f_gpu_efficient) {
                ++m_gpu_frequency_clipped;
            }
            f_request = std::min(f_request, m_resolved_f_gpu_max);
            f_request = std::max(f_request, m_resolved_f_gpu_efficient);

            if (phi >= 0.5) {
                if (!std::isnan(gpu_utilization) &&
                    gpu_utilization == 0) {
                    if (m_gpu_idle_timer.at(domain_idx) > 0) {
                        m_gpu_idle_timer.at(domain_idx) = m_gpu_idle_timer.at(domain_idx) - 1;
                    }
                }
                else {
                    // If no activity has been observed for a number of samples
                    // IDLE_SAMPLE_COUNT we assume it is safe to reduce the frequency
                    // to a minimum value.
                    m_gpu_idle_timer.at(domain_idx) = IDLE_SAMPLE_COUNT;
                }

                if (m_gpu_idle_timer.at(domain_idx) <= 0) {
                    f_request = m_freq_gpu_min;
                    m_gpu_idle_samples.at(domain_idx) = m_gpu_idle_samples.at(domain_idx) + 1;
                }
            }

            // Store frequency request
            gpu_freq_request.push_back(f_request);
        }

        // Tracking logic.  This is not needed for any performance reason,
        // but does provide useful metrics for tracking agent behavior.  This
        // may be removed when GPU regions are added to GEOPM.
        if (!gpu_scoped_core_activity.empty()) {
            for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
                if (gpu_scoped_core_activity.at(domain_idx) >= M_GPU_ACTIVITY_CUTOFF) {
                    // ROI proxy tracking
                    m_gpu_active_region_stop.at(domain_idx) = 0;
                    if (m_gpu_active_region_start.at(domain_idx) == 0) {
                        m_gpu_active_region_start.at(domain_idx) = m_time.value;
                        m_gpu_active_energy_start.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
                    }

                    // GPU on time tracking
                    if (m_time.value > m_prev_time) {
                        m_gpu_on_time.at(domain_idx) += m_time.value - m_prev_time;
                    }

                    // TODO: handle roll-over more gracefully than dropping a sample
                    if (m_gpu_energy.at(domain_idx).value > m_prev_gpu_energy.at(domain_idx)) {
                        m_gpu_on_energy.at(domain_idx) += m_gpu_energy.at(domain_idx).value - m_prev_gpu_energy.at(domain_idx);
                    }
                }
                else {
                    // ROI proxy tracking
                    if (m_gpu_active_region_start.at(domain_idx) != 0 &&
                        m_gpu_active_region_stop.at(domain_idx) == 0) {
                        m_gpu_active_region_stop.at(domain_idx) = m_time.value;
                        m_gpu_active_energy_stop.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
                    }
                }
            }
        }

        // set frequency control per gpu
        for (int domain_idx = 0; domain_idx < m_agent_domain_count; ++domain_idx) {
            if (gpu_freq_request.at(domain_idx) !=
                m_gpu_freq_min_control.at(domain_idx).last_setting ||
                gpu_freq_request.at(domain_idx) !=
                m_gpu_freq_max_control.at(domain_idx).last_setting) {

                m_platform_io.adjust(m_gpu_freq_min_control.at(domain_idx).batch_idx,
                                     gpu_freq_request.at(domain_idx));
                m_gpu_freq_min_control.at(domain_idx).last_setting =
                                     gpu_freq_request.at(domain_idx);

                m_platform_io.adjust(m_gpu_freq_max_control.at(domain_idx).batch_idx,
                                     gpu_freq_request.at(domain_idx));
                m_gpu_freq_max_control.at(domain_idx).last_setting =
                                     gpu_freq_request.at(domain_idx);
                ++m_gpu_frequency_requests;
                m_do_write_batch = true;
            }
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
        GEOPM_DEBUG_ASSERT(out_sample.size() == M_NUM_SAMPLE,
                           "GPUActivityAgent::sample_platform(): sample output vector incorrectly sized");

        // Collect latest signal values
        for (int domain_idx = 0; domain_idx < m_agent_domain_count; ++domain_idx) {
            m_gpu_core_activity.at(domain_idx).value = m_platform_io.sample(m_gpu_core_activity.at(
                                                                               domain_idx).batch_idx);
            m_gpu_utilization.at(domain_idx).value = m_platform_io.sample(m_gpu_utilization.at(
                                                                          domain_idx).batch_idx);
        }

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_prev_gpu_energy.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
            m_gpu_energy.at(domain_idx).value = m_platform_io.sample(m_gpu_energy.at(
                                                                     domain_idx).batch_idx);
        }

        m_prev_time = m_time.value;
        m_time.value = m_platform_io.sample(m_time.batch_idx);
    }

    // Wait for the remaining cycle time to keep Controller loop cadence
    void GPUActivityAgent::wait(void)
    {
        m_waiter->wait();
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > GPUActivityAgent::report_header(void) const
    {
        return {{"Wait time (sec)", std::to_string(m_waiter->period())}};
    }

    // Adds number of frquency requests to the per-node section of the report
    std::vector<std::pair<std::string, std::string> > GPUActivityAgent::report_host(void) const
    {
        std::vector<std::pair<std::string, std::string> > result;

        result.push_back({"Agent Domain", m_platform_topo.domain_type_to_name(m_agent_domain)});
        result.push_back({"GPU Frequency Requests", std::to_string(m_gpu_frequency_requests)});
        result.push_back({"GPU Clipped Frequency Requests", std::to_string(m_gpu_frequency_clipped)});
        result.push_back({"Resolved Max Frequency", std::to_string(m_resolved_f_gpu_max)});
        result.push_back({"Resolved Efficient Frequency", std::to_string(m_resolved_f_gpu_efficient)});
        result.push_back({"Resolved Frequency Range", std::to_string(m_f_range)});

        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            double energy_stop = m_gpu_active_energy_stop.at(domain_idx);
            double energy_start = m_gpu_active_energy_start.at(domain_idx);
            double region_start =  m_gpu_active_region_start.at(domain_idx);
            double region_stop = m_gpu_active_region_stop.at(domain_idx);
            // If the end of the active region was never seen assume that the end of the run
            // is the end of the region.
            if (m_gpu_active_region_stop.at(domain_idx) == 0.0) {
                region_stop = m_time.value;
            }
            // Either the last energy value was at the point of roll-over or the GPU Utilization
            // was above cutoff for the entire run.  In either case grabbing the last sample
            // value is a safe decision (if rollover it'll be 0 still, if not i'll be the end of
            // run value).  We have to account for the case where no GPU activity was seen however,
            // so we check for an active region time of 0 (i.e. region start != region stop)
            if (m_gpu_active_energy_stop.at(domain_idx) == 0 &&
                region_stop != region_start) {
                energy_stop = m_gpu_energy.at(domain_idx).value;
            }

            result.push_back({"GPU " + std::to_string(domain_idx) +
                              " Active Region Energy", std::to_string(energy_stop - energy_start)});
            result.push_back({"GPU " + std::to_string(domain_idx) +
                              " Active Region Time", std::to_string(region_stop - region_start)});
            result.push_back({"GPU " + std::to_string(domain_idx) +
                              " On Energy", std::to_string(m_gpu_on_energy.at(domain_idx))});
            result.push_back({"GPU " + std::to_string(domain_idx) +
                              " On Time", std::to_string(m_gpu_on_time.at(domain_idx))});
        }

        result.push_back({"Agent Idle Samples Required to Request Minimum Frequency", std::to_string(IDLE_SAMPLE_COUNT)});
        result.push_back({"Agent Idle Time (estimate in seconds) Required to Request Minimum Frequency", std::to_string(IDLE_SAMPLE_COUNT * m_waiter->period())});
        for (int domain_idx = 0; domain_idx < m_agent_domain_count; ++domain_idx) {
            result.push_back({"GPU Chip " + std::to_string(domain_idx) +
                              " Idle Agent Actions", std::to_string(m_gpu_idle_samples.at(domain_idx))});
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

    void GPUActivityAgent::enforce_policy(const std::vector<double> &policy) const
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
        return {"GPU_PHI"};
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> GPUActivityAgent::sample_names(void)
    {
        return {};
    }
}
