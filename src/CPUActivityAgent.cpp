/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "CPUActivityAgent.hpp"

#include <algorithm>
#include <cmath>
#include <cassert>
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
        , m_core_batch_writes(0)
        , m_uncore_frequency_requests(0)
        , m_uncore_frequency_clamped(0)
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
        m_resolved_f_uncore_max = m_freq_uncore_max;

        if (level == 0) {
            init_platform_io();
            init_constconfig_io();
        }
    }

    void CPUActivityAgent::init_platform_io(void)
    {
        int scalability_signal_domain = m_platform_io.signal_domain_type("MSR::CPU_SCALABILITY_RATIO");

        // If the frequency control domain does not contain the scalabilty domain
        // (i.e. the scalability domain is coarser than the freq domain) use the
        // scalability domain for frequency control.
        if (!m_platform_topo.is_nested_domain(scalability_signal_domain,
                                              m_freq_ctl_domain_type)) {

#ifdef GEOPM_DEBUG
            std::cerr << "CPUActivityAgent::" + std::string(__func__) +
                          "():MSR::CPU_SCALABILITY_RATIO domain (" +
                          std::to_string(scalability_signal_domain) +
                          ") is a coarser granularity than the CPU frequency control domain (" +
                          std::to_string(m_freq_ctl_domain_type) + ").";
#endif

            // Set Freq gov domain.
            m_freq_governor->set_domain_type(scalability_signal_domain);

            // update member vars
            m_freq_ctl_domain_type = m_freq_governor->frequency_domain_type();
            m_num_freq_ctl_domain = m_platform_topo.num_domain(m_freq_ctl_domain_type);
        }

        m_freq_governor->init_platform_io();

        m_freq_core_min = m_freq_governor->get_frequency_min();
        m_freq_core_max = m_freq_governor->get_frequency_max();

        for (int domain_idx = 0; domain_idx < m_num_freq_ctl_domain; ++domain_idx) {
            m_core_scal.push_back({m_platform_io.push_signal("MSR::CPU_SCALABILITY_RATIO",
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

    void CPUActivityAgent::init_constconfig_io()
    {
        m_qm_max_rate = {};
        const auto ALL_NAMES = m_platform_io.signal_names();

        // F efficient values
        std::string fe_constconfig = "CONST_CONFIG::CPU_FREQUENCY_EFFICIENT_HIGH_INTENSITY";
        if (ALL_NAMES.count(fe_constconfig) != 0) {
            m_freq_core_efficient = m_platform_io.read_signal(fe_constconfig, GEOPM_DOMAIN_BOARD, 0);
        }
        else {
            m_freq_core_efficient = m_freq_core_min;
        }

        fe_constconfig = "CONST_CONFIG::CPU_UNCORE_FREQUENCY_EFFICIENT_HIGH_INTENSITY";
        if (ALL_NAMES.count(fe_constconfig) != 0) {
            m_freq_uncore_efficient = m_platform_io.read_signal(fe_constconfig, GEOPM_DOMAIN_BOARD, 0);
        }
        else {
            m_freq_uncore_efficient = m_freq_uncore_min;
        }

        // Grab all (uncore frequency, max memory bandwidth) pairs
        for (int entry_idx = 0; entry_idx < (int)ALL_NAMES.size(); ++entry_idx) {
            const std::string KEY_NAME = "CONST_CONFIG::CPU_UNCORE_FREQUENCY_" +
                                          std::to_string(entry_idx);
            const std::string VAL_NAME = "CONST_CONFIG::CPU_UNCORE_MAX_MEMORY_BANDWIDTH_" +
                                          std::to_string(entry_idx);
            if (ALL_NAMES.find(KEY_NAME) != ALL_NAMES.end() &&
                ALL_NAMES.find(VAL_NAME) != ALL_NAMES.end()) {
                double uncore_freq = m_platform_io.read_signal(KEY_NAME, GEOPM_DOMAIN_BOARD, 0);
                double max_mem_bw = m_platform_io.read_signal(VAL_NAME, GEOPM_DOMAIN_BOARD, 0);
                if (!std::isnan(uncore_freq) && !std::isnan(max_mem_bw) &&
                    uncore_freq != 0 && max_mem_bw != 0) {
                    m_qm_max_rate[uncore_freq] = max_mem_bw;
                }
            }
        }

        if (m_qm_max_rate.empty()) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): ConstConfigIO file did not contain" +
                            " memory bandwidth information.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    // Validate incoming policy and configure default policy requests.
    void CPUActivityAgent::validate_policy(std::vector<double> &in_policy) const
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY,
                           "CPUActivityAgent::" + std::string(__func__) +
                           "(): policy vector not correctly sized.  Expected  " +
                           std::to_string(M_NUM_POLICY) + ", actual: " +
                           std::to_string(in_policy.size()));

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

        // Calculate new frequency range values
        double f_core_range = m_freq_core_max - m_freq_core_efficient;
        double f_uncore_range = m_freq_uncore_max - m_freq_uncore_efficient;

        double phi = in_policy[M_POLICY_CPU_PHI];

        // Default phi = 0.5 case is full fe to fmax range
        // Core
        m_resolved_f_core_max = m_freq_core_max;
        m_resolved_f_core_efficient = m_freq_core_efficient;
        // Uncore
        m_resolved_f_uncore_max = m_freq_uncore_max;
        m_resolved_f_uncore_efficient = m_freq_uncore_efficient;

        // If phi is not 0.5 we move into the energy or performance biased behavior
        if (phi > 0.5) {
            // Energy Biased.  Scale F_max down to F_efficient based upon phi value
            // Active region phi usage
            m_resolved_f_core_max = std::max(m_freq_core_efficient, m_freq_core_max -
                                                                    f_core_range *
                                                                    (phi-0.5) / 0.5);

            m_resolved_f_uncore_max = std::max(m_freq_uncore_efficient, m_freq_uncore_max -
                                                                        f_uncore_range *
                                                                        (phi-0.5) / 0.5);
        }
        else if (phi < 0.5) {
            // Perf Biased.  Scale F_efficient up to F_max based upon phi value
            // Active region phi usage
            m_resolved_f_core_efficient = std::min(m_freq_core_max, m_freq_core_efficient +
                                                                    f_core_range *
                                                                    (0.5-phi) / 0.5);

            m_resolved_f_uncore_efficient = std::min(m_freq_uncore_max, m_freq_uncore_efficient +
                                                                        f_uncore_range *
                                                                        (0.5-phi) / 0.5);
        }

        //Update Policy
        m_freq_governor->validate_policy(m_resolved_f_core_efficient,
                                         m_resolved_f_core_max);
        m_freq_governor->set_frequency_bounds(m_resolved_f_core_efficient,
                                              m_resolved_f_core_max);

        f_core_range = m_resolved_f_core_max - m_resolved_f_core_efficient;
        f_uncore_range = m_resolved_f_uncore_max - m_resolved_f_uncore_efficient;

        // Per package freq
        std::vector<double> uncore_freq_request;

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
            // For now only L3 bandwidth metric is used.
            double uncore_req = m_resolved_f_uncore_efficient + f_uncore_range * scalability_uncore;

            // Clamp uncore request within policy limits
            if (uncore_req > m_resolved_f_uncore_max || uncore_req < m_resolved_f_uncore_efficient) {
                ++m_uncore_frequency_clamped;
            }
            uncore_req = std::max(m_resolved_f_uncore_efficient, uncore_req);
            uncore_req = std::min(m_resolved_f_uncore_max, uncore_req);
            uncore_freq_request.push_back(uncore_req);
        }

        // Per core freq
        std::vector<double> core_freq_request;

        for (int domain_idx = 0; domain_idx < m_num_freq_ctl_domain; ++domain_idx) {
            //////////////////////////////////
            // Core Scalability Measurement //
            //////////////////////////////////
            double scalability = (double) m_core_scal.at(domain_idx).value;
            if (std::isnan(scalability)) {
                scalability = 1.0;
            }

            double core_req = m_resolved_f_core_efficient + f_core_range * scalability;
            core_freq_request.push_back(core_req);
        }

        m_freq_governor->adjust_platform(core_freq_request);
        // Track number of core requests
        if (m_freq_governor->do_write_batch()) {
            ++m_core_batch_writes;
        }

        // Set per package controls
        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            if (std::isnan(uncore_freq_request.at(domain_idx))) {
                uncore_freq_request.at(domain_idx) = m_freq_uncore_max;
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

        result.push_back({"Core Batch Writes",
                          std::to_string(m_core_batch_writes)});
        result.push_back({"Core Frequency Requests Clamped",
                          std::to_string(m_freq_governor->get_clamp_count())});
        result.push_back({"Uncore Frequency Requests",
                          std::to_string(m_uncore_frequency_requests)});
        result.push_back({"Uncore Frequency Requests Clamped",
                          std::to_string(m_uncore_frequency_clamped)});
        result.push_back({"Resolved Maximum Core Frequency",
                          std::to_string(m_resolved_f_core_max)});
        result.push_back({"Resolved Efficient Core Frequency",
                          std::to_string(m_resolved_f_core_efficient)});
        result.push_back({"Resolved Core Frequency Range",
                          std::to_string(m_resolved_f_core_max - m_resolved_f_core_efficient)});
        result.push_back({"Resolved Maximum Uncore Frequency",
                          std::to_string(m_resolved_f_uncore_max)});
        result.push_back({"Resolved Efficient Uncore Frequency",
                          std::to_string(m_resolved_f_uncore_efficient)});
        result.push_back({"Resolved Uncore Frequency Range",
                          std::to_string(m_resolved_f_uncore_max - m_resolved_f_uncore_efficient)});
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
        std::vector<std::string> names{"CPU_PHI"};
        return names;
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> CPUActivityAgent::sample_names(void)
    {
        return {};
    }
}
