/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "CPUActivityAgent.hpp"

#include <cmath>
#include <cassert>
#include <algorithm>
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
#include "FrequencyGovernor.hpp"

namespace geopm
{
    CPUActivityAgent::CPUActivityAgent()
        : CPUActivityAgent(platform_io(), platform_topo(), FrequencyGovernor::make_shared())
    {

    }

    CPUActivityAgent::CPUActivityAgent(PlatformIO &plat_io,
                                       const PlatformTopo &topo,
                                       std::shared_ptr<FrequencyGovernor> gov)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(0.010) // 10ms wait default
        , M_POLICY_PHI_DEFAULT(0.5)
        , M_NUM_PACKAGE(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
        , m_do_write_batch(false)
        , m_do_send_policy(true)
        , m_freq_governor(gov)
        , m_freq_ctl_domain_type(m_freq_governor->frequency_domain_type())
        , m_num_freq_ctl_domain(m_platform_topo.num_domain(m_freq_ctl_domain_type))
        , m_core_frequency_requests(0)
        , m_uncore_frequency_requests(0)
        , m_core_frequency_clipped(0)
        , m_uncore_frequency_clipped(0)
        , m_resolved_f_uncore_efficient(0)
        , m_resolved_f_uncore_max(0)
        , m_resolved_f_core_efficient(0)
        , m_resolved_f_core_max(0)
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void CPUActivityAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        // These are not currently guaranteed to be the system uncore min and max,
        // just what the user/admin has previously set.
        m_freq_uncore_min = m_platform_io.read_signal("CPU_UNCORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0);
        m_freq_uncore_max = m_platform_io.read_signal("CPU_UNCORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0);
        m_freq_core_min = m_freq_governor->get_frequency_min();
        m_freq_core_max = m_freq_governor->get_frequency_max();

        if (level == 0) {
            m_freq_governor->init_platform_io();
            init_platform_io();
        }
    }

    void CPUActivityAgent::init_platform_io(void)
    {
        for (int domain_idx = 0; domain_idx < m_num_freq_ctl_domain; ++domain_idx) {
            m_core_scal.push_back({m_platform_io.push_signal("MSR::CPU_SCALABILITY_RATIO",
                                                             m_freq_ctl_domain_type,
                                                             domain_idx), NAN});
            m_core_freq_control.push_back({m_platform_io.push_control("CPU_FREQUENCY_MAX_CONTROL",
                                                                      m_freq_ctl_domain_type,
                                                                      domain_idx), NAN});
        }

        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            m_qm_rate.push_back({m_platform_io.push_signal("MSR::QM_CTR_SCALED_RATE",
                                                           GEOPM_DOMAIN_PACKAGE,
                                                           domain_idx), NAN});

            m_uncore_freq_status.push_back({m_platform_io.push_signal("CPU_UNCORE_FREQUENCY_STATUS",
                                                                      GEOPM_DOMAIN_PACKAGE,
                                                                      domain_idx), NAN});

            m_uncore_freq_min_control.push_back({m_platform_io.push_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL",
                                                                            GEOPM_DOMAIN_PACKAGE,
                                                                            domain_idx), -1});
            m_uncore_freq_max_control.push_back({m_platform_io.push_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL",
                                                                            GEOPM_DOMAIN_PACKAGE,
                                                                            domain_idx), -1});
        }

        // Configuration of QM_CTR must match QM_CTR config used for tuning/training data.
        // Assign all cores to resource monitoring association ID 0.  This allows for
        // monitoring the resource usage of all cores.
        m_platform_io.write_control("MSR::PQR_ASSOC:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
        // Assign the resource monitoring ID for QM Events to match the per core resource
        // association ID above (0)
        m_platform_io.write_control("MSR::QM_EVTSEL:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
        // Select monitoring event ID 0x2 - Total Memory Bandwidth Monitoring.  This
        // is used to determine the Xeon Uncore utilization.
        m_platform_io.write_control("MSR::QM_EVTSEL:EVENT_ID", GEOPM_DOMAIN_BOARD, 0, 2);
    }

    // Validate incoming policy and configure default policy requests.
    void CPUActivityAgent::validate_policy(std::vector<double> &in_policy) const
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY,
                           "CPUActivityAgent::" + std::string(__func__) +
                           "(): policy vector not correctly sized.  Expected  " +
                           std::to_string(M_NUM_POLICY) + ", actual: " +
                           std::to_string(in_policy.size()));

        // Check for NAN to set default values for policy
        if (std::isnan(in_policy[M_POLICY_CPU_FREQ_MAX])) {
            in_policy[M_POLICY_CPU_FREQ_MAX] = m_freq_core_max;
        }

        // Check for NAN to set default values for policy
        if (std::isnan(in_policy[M_POLICY_CPU_FREQ_EFFICIENT])) {
            in_policy[M_POLICY_CPU_FREQ_EFFICIENT] = m_freq_core_min;
        }

        //////////////////////////
        //UNCORE POLICY CHECKING//
        //////////////////////////
        if (std::isnan(in_policy[M_POLICY_UNCORE_FREQ_MAX])) {
            in_policy[M_POLICY_UNCORE_FREQ_MAX] = m_freq_uncore_max;
        }
        if (std::isnan(in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT])) {
            in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT] = m_freq_uncore_min;
        }

        // When the policy is all NaNs, this check also verifies that
        // the system was not left in a bad state with regard to the
        // UNCORE_FREQUENCY_<MAX/MIN>_CONTROLs.
        if (in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT] > in_policy[M_POLICY_UNCORE_FREQ_MAX]) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "():CPU_UNCORE_FREQ_EFFICIENT (" +
                            std::to_string(in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT]) +
                            ") value exceeds CPU_UNCORE_FREQ_MAX (" +
                            std::to_string(in_policy[M_POLICY_UNCORE_FREQ_MAX]) +
                            ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // If no phi value is provided assume the default behavior.
        if (std::isnan(in_policy[M_POLICY_CPU_PHI])) {
            in_policy[M_POLICY_CPU_PHI] = M_POLICY_PHI_DEFAULT;
        }

        if (in_policy[M_POLICY_CPU_PHI] < 0.0 ||
            in_policy[M_POLICY_CPU_PHI] > 1.0) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): POLICY_CPU_PHI value out of range: " +
                            std::to_string(in_policy[M_POLICY_CPU_PHI]) + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        double f_core_max = in_policy[M_POLICY_CPU_FREQ_MAX];
        double f_core_efficient = in_policy[M_POLICY_CPU_FREQ_EFFICIENT];
        double f_core_range = f_core_max - f_core_efficient;

        double f_uncore_max = in_policy[M_POLICY_UNCORE_FREQ_MAX];
        double f_uncore_efficient = in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT];
        double f_uncore_range = f_uncore_max - f_uncore_efficient;

        double phi = in_policy[M_POLICY_CPU_PHI];

        // If phi is not 0.5 we move into the energy or performance biased behavior
        if (phi > 0.5) {
            // Energy Biased.  Scale F_max down to F_efficient based upon phi value
            // Active region phi usage
            f_core_max = std::max(f_core_efficient, f_core_max -
                                                    f_core_range * (phi-0.5) / 0.5);
            f_uncore_max = std::max(f_uncore_efficient, f_uncore_max -
                                                        f_uncore_range * (phi-0.5) / 0.5);
        }
        else if (phi < 0.5) {
            // Perf Biased.  Scale F_efficient up to F_max based upon phi value
            // Active region phi usage
            f_core_efficient = std::min(f_core_max, f_core_efficient +
                                                    f_core_range * (0.5-phi) / 0.5);

            f_uncore_efficient = std::min(f_uncore_max, f_uncore_efficient +
                                                        f_uncore_range * (0.5-phi) / 0.5);
        }
        //Update Policy
        in_policy[M_POLICY_CPU_FREQ_MAX] = f_core_max;
        in_policy[M_POLICY_CPU_FREQ_EFFICIENT] = f_core_efficient;
        in_policy[M_POLICY_UNCORE_FREQ_MAX] = f_uncore_max;
        in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT] = f_uncore_efficient;

        m_freq_governor->validate_policy(in_policy[M_POLICY_CPU_FREQ_EFFICIENT],
                                         in_policy[M_POLICY_CPU_FREQ_MAX]);
        m_freq_governor->set_frequency_bounds(in_policy[M_POLICY_CPU_FREQ_EFFICIENT],
                                              in_policy[M_POLICY_CPU_FREQ_MAX]);

        // Validate all (uncore frequency, max memory bandwidth) pairs
        // Example policy values parsed here:
        //
        // UNCORE_FREQ_0": 1800000000.0,
        // "MAX_MEMORY_BANDWIDTH_0": 108066060000.0,
        // "CPU_UNCORE_FREQ_1": 1900000000.0,
        // "MAX_MEMORY_BANDWIDTH_1": 116333135000.0,
        // ...
        // CPU_UNCORE_FREQ_<#>": 2400000000.0,
        // "MAX_MEMORY_BANDWIDTH_<#>": 106613110000.0
        std::set<double> policy_uncore_freqs;
        for (auto it = in_policy.begin() + M_POLICY_FIRST_UNCORE_FREQ;
             it != in_policy.end() && std::next(it) != in_policy.end(); std::advance(it, 2)) {
            auto mapped_mem_bw = *(it + 1);
            auto uncore_freq = (*it);
            if (!std::isnan(uncore_freq)) {
                if (std::isnan(mapped_mem_bw)) {
                    throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                    "(): mapped CPU_UNCORE_FREQUENCY with no max memory bandwidth.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                // Just make sure the frequency does not have multiple definitions.
                if (!policy_uncore_freqs.insert(uncore_freq).second) {
                    throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                    "(): policy has multiple entries for CPU_UNCORE_FREQUENCY " +
                                    std::to_string(uncore_freq),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            else if (!std::isnan(mapped_mem_bw)) {
                throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                "(): policy maps a NaN CPU_UNCORE_FREQUENCY with max memory bandwidth: " +
                                std::to_string(mapped_mem_bw),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    // Distribute incoming policy to children
    void CPUActivityAgent::split_policy(const std::vector<double>& in_policy,
                                        std::vector<std::vector<double> >& out_policy)
    {
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool CPUActivityAgent::do_send_policy(void) const
    {
        return m_do_send_policy;
    }

    void CPUActivityAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                            std::vector<double>& out_sample)
    {

    }

    // Indicate whether to send samples up to the parent
    bool CPUActivityAgent::do_send_sample(void) const
    {
        return false;
    }

    void CPUActivityAgent::adjust_platform(const std::vector<double>& in_policy)
    {
        m_do_send_policy = false;
        m_do_write_batch = false;

        if (m_qm_max_rate.empty()) {
            for (auto it = in_policy.begin() + M_POLICY_FIRST_UNCORE_FREQ;
                 it != in_policy.end() && std::next(it) != in_policy.end();
                 std::advance(it, 2)) {

                auto uncore_freq = (*it);
                auto max_mem_bw = *(it + 1);
                if (!std::isnan(uncore_freq)) {
                    // Not valid to have NAN max mem bw for uncore freq.
                    GEOPM_DEBUG_ASSERT(!std::isnan(max_mem_bw),
                                       "mapped CPU_UNCORE_FREQUENCY with no max memory bandwidth assigned.");
                    m_qm_max_rate[uncore_freq] = max_mem_bw;
                }
            }

            if (m_qm_max_rate.empty()) {
                throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                "(): CPUActivityAgent policy did not contain" +
                                " memory bandwidth characterization.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }

        // Per package freq
        std::vector<double> uncore_freq_request;
        m_resolved_f_uncore_efficient = in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT];
        m_resolved_f_uncore_max = in_policy[M_POLICY_UNCORE_FREQ_MAX];
        double f_uncore_range = in_policy[M_POLICY_UNCORE_FREQ_MAX] - in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT];

        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            double uncore_freq = (double) m_uncore_freq_status.at(domain_idx).value;

            /////////////////////////////////////////////
            // L3 Total External Bandwidth Measurement //
            /////////////////////////////////////////////
            // Get max mem. bandwidth for uncore_freq. There may be uncore
            // frequencies for which an exact match doesn't exist. To handle
            // this case, we grab the entry prior to upper_bound() (as long as
            // it's not the first entry), in other words, the last entry that
            // is <= uncore_freq.
            auto qm_max_itr = m_qm_max_rate.upper_bound(uncore_freq);
            if (qm_max_itr != m_qm_max_rate.begin())
                --qm_max_itr;

            double scalability_uncore = 1.0;

            // Handle divided by zero, either numerator or
            // denominator being NAN
            if (!std::isnan(m_qm_rate.at(domain_idx).value) &&
                !std::isnan(qm_max_itr->second) &&
                qm_max_itr->second != 0) {
                scalability_uncore = (double) m_qm_rate.at(domain_idx).value /
                                         qm_max_itr->second;
            }

            // L3 usage, Network Traffic, HBM, and PCIE (GPUs) all use the uncore.
            // Eventually all these components should be considered when scaling
            // the uncore frequency in the efficient - performant range.
            // A more robust/future proof solution may be to directly query uncore
            // counters that indicate utilization (when/if available).
            // For now only L3 bandwith metric is used.
            double uncore_req = m_resolved_f_uncore_efficient + f_uncore_range * scalability_uncore;

            // Clip uncore request within policy limits
            if (uncore_req > m_resolved_f_uncore_max || uncore_req < m_resolved_f_uncore_efficient) {
                ++m_uncore_frequency_clipped;
            }
            uncore_req = std::max(m_resolved_f_uncore_efficient, uncore_req);
            uncore_req = std::min(m_resolved_f_uncore_max, uncore_req);
            uncore_freq_request.push_back(uncore_req);
        }

        // Per core freq
        std::vector<double> core_freq_request;
        m_resolved_f_core_efficient = in_policy[M_POLICY_CPU_FREQ_EFFICIENT];
        m_resolved_f_core_max = in_policy[M_POLICY_CPU_FREQ_MAX];
        double f_core_range = in_policy[M_POLICY_CPU_FREQ_MAX] - in_policy[M_POLICY_CPU_FREQ_EFFICIENT];

        for (int domain_idx = 0; domain_idx < m_num_freq_ctl_domain; ++domain_idx) {
            //////////////////////////////////
            // Core Scalability Measurement //
            //////////////////////////////////
            double scalability = (double) m_core_scal.at(domain_idx).value;
            if (std::isnan(scalability)) {
                scalability = 1.0;
            }

            double core_req = m_resolved_f_core_efficient + f_core_range * scalability;

            if (std::isnan(core_req)) {
                core_req = in_policy[M_POLICY_CPU_FREQ_MAX];
            }
            else if (core_req > m_resolved_f_core_max || core_req < m_resolved_f_core_efficient) {
                // Clip core request within policy limits
                ++m_core_frequency_clipped;
            }

            core_freq_request.push_back(core_req);

            // Track number of core requests
            if (std::isnan(m_core_freq_control.at(domain_idx).last_setting) ||
                core_req != m_core_freq_control.at(domain_idx).last_setting) {
                ++m_core_frequency_requests;
            }
            m_core_freq_control.at(domain_idx).last_setting = core_req;
        }

        m_freq_governor->adjust_platform(core_freq_request);

        // Set per package controls
        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            if (std::isnan(uncore_freq_request.at(domain_idx))) {
                uncore_freq_request.at(domain_idx) = in_policy[M_POLICY_UNCORE_FREQ_MAX];
            }

            if (uncore_freq_request.at(domain_idx) !=
                m_uncore_freq_min_control.at(domain_idx).last_setting ||
                uncore_freq_request.at(domain_idx) !=
                m_uncore_freq_max_control.at(domain_idx).last_setting) {
                // Adjust
                m_platform_io.adjust(m_uncore_freq_min_control.at(domain_idx).batch_idx,
                                    uncore_freq_request.at(domain_idx));

                m_platform_io.adjust(m_uncore_freq_max_control.at(domain_idx).batch_idx,
                                    uncore_freq_request.at(domain_idx));

                // Save the value for future comparison
                m_uncore_freq_min_control.at(domain_idx).last_setting = uncore_freq_request.at(domain_idx);
                m_uncore_freq_max_control.at(domain_idx).last_setting = uncore_freq_request.at(domain_idx);
                ++m_uncore_frequency_requests;

                m_do_write_batch = true;
            }
        }
    }

    // If controls have a valid updated value write them.
    bool CPUActivityAgent::do_write_batch(void) const
    {
        return m_do_write_batch || m_freq_governor->do_write_batch();
    }

    // Read signals from the platform and calculate samples to be sent up
    void CPUActivityAgent::sample_platform(std::vector<double> &out_sample)
    {
        GEOPM_DEBUG_ASSERT(out_sample.size() == M_NUM_SAMPLE,
                           "CPUActivityAgent::" + std::string(__func__) +
                           "(): sample vector not correctly sized.  Expected  " +
                           std::to_string(M_NUM_SAMPLE) + ", actual: " +
                           std::to_string(out_sample.size()));

        // Collect latest signal values
        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            // Frequency signals
            m_uncore_freq_status.at(domain_idx).value = m_platform_io.sample(m_uncore_freq_status.at(domain_idx).batch_idx);

            // Uncore steering signals
            m_qm_rate.at(domain_idx).value = m_platform_io.sample(m_qm_rate.at(domain_idx).batch_idx);
        }

        for (int domain_idx = 0; domain_idx < m_num_freq_ctl_domain; ++domain_idx) {
            // Core steering signals
            m_core_scal.at(domain_idx).value = m_platform_io.sample(m_core_scal.at(domain_idx).batch_idx);
        }
    }

    // Wait for the remaining cycle time to keep Controller loop cadence
    void CPUActivityAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > CPUActivityAgent::report_header(void) const
    {
        return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
    }

    // Adds number of frquency requests to the per-node section of the report
    std::vector<std::pair<std::string, std::string> > CPUActivityAgent::report_host(void) const
    {
        std::vector<std::pair<std::string, std::string> > result;

        result.push_back({"Core Frequency Requests", std::to_string(m_core_frequency_requests)});
        result.push_back({"Core Clipped Frequency Requests", std::to_string(m_core_frequency_clipped)});
        result.push_back({"Uncore Frequency Requests", std::to_string(m_uncore_frequency_requests)});
        result.push_back({"Uncore Clipped Frequency Requests", std::to_string(m_uncore_frequency_clipped)});
        result.push_back({"Resolved Maximum Core Frequency", std::to_string(m_resolved_f_core_max)});
        result.push_back({"Resolved Efficient Core Frequency", std::to_string(m_resolved_f_core_efficient)});
        result.push_back({"Resolved Core Frequency Range", std::to_string(m_resolved_f_core_max - m_resolved_f_core_efficient)});
        result.push_back({"Resolved Maximum Uncore Frequency", std::to_string(m_resolved_f_uncore_max)});
        result.push_back({"Resolved Efficient Uncore Frequency", std::to_string(m_resolved_f_uncore_efficient)});
        result.push_back({"Resolved Uncore Frequency Range", std::to_string(m_resolved_f_uncore_max - m_resolved_f_uncore_efficient)});
        return result;
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > CPUActivityAgent::report_region(void) const
    {
        return {};
    }

    // Adds trace columns signals of interest
    std::vector<std::string> CPUActivityAgent::trace_names(void) const
    {
        return {};
    }

    // Updates the trace with values for signals from this Agent
    void CPUActivityAgent::trace_values(std::vector<double> &values)
    {
    }

    void CPUActivityAgent::enforce_policy(const std::vector<double> &policy) const
    {

    }

    std::vector<std::function<std::string(double)> > CPUActivityAgent::trace_formats(void) const
    {
        return {};
    }

    // Name used for registration with the Agent factory
    std::string CPUActivityAgent::plugin_name(void)
    {
        return "cpu_activity";
    }

    // Used by the factory to create objects of this type
    std::unique_ptr<Agent> CPUActivityAgent::make_plugin(void)
    {
        return geopm::make_unique<CPUActivityAgent>();
    }

    // Describes expected policies to be provided by the resource manager or user
    std::vector<std::string> CPUActivityAgent::policy_names(void)
    {
        std::vector<std::string> names{"CPU_FREQ_MAX", "CPU_FREQ_EFFICIENT", "CPU_UNCORE_FREQ_MAX",
                                       "CPU_UNCORE_FREQ_EFFICIENT", "CPU_PHI"};
        names.reserve(M_NUM_POLICY);

        for (size_t i = 0; names.size() < M_NUM_POLICY; ++i) {
            names.emplace_back("CPU_UNCORE_FREQ_" + std::to_string(i));
            names.emplace_back("MAX_MEMORY_BANDWIDTH_" + std::to_string(i));
        }
        return names;
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> CPUActivityAgent::sample_names(void)
    {
        return {};
    }
}
