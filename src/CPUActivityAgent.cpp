/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

#include "PlatformIOProf.hpp"

namespace geopm
{
    CPUActivityAgent::CPUActivityAgent()
        : CPUActivityAgent(platform_io(), platform_topo())
    {

    }

    CPUActivityAgent::CPUActivityAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(0.010) // 10ms wait default
        , M_POLICY_PHI_DEFAULT(0.5)
        , M_NUM_PACKAGE(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
        , M_NUM_CORE(m_platform_topo.num_domain(GEOPM_DOMAIN_CORE))
        , m_do_write_batch(false)
        //TODO: change to a policy as this is not guaranteed across SKUs
        //      and families
        , m_qm_max_rate({
                                {1.2e9, 4.56E+10},
                                {1.3e9, 6.53E+10},
                                {1.4e9, 7.42E+10},
                                {1.5e9, 7.71E+10},
                                {1.6e9, 8.40E+10},
                                {1.7e9, 8.87E+10},
                                {1.8e9, 9.28E+10},
                                {1.9e9, 9.80E+10},
                                {2.0e9, 1.02E+11},
                                {2.1e9, 1.01E+11},
                                {2.2e9, 1.04E+11},
                                {2.3e9, 1.04E+11},
                                {2.4e9, 1.05E+11}
                                })
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void CPUActivityAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_core_frequency_requests = 0;
        m_uncore_frequency_requests = 0;
        m_network_normalized_frequency_requests = 0;

        // These are not currently guaranteed to be the system uncore min and max,
        // just what the user/admin has previously set.
        m_freq_uncore_min = m_platform_io.read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0);
        m_freq_uncore_max = m_platform_io.read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0);

        init_platform_io();
    }

    void CPUActivityAgent::init_platform_io(void)
    {
        for (int domain_idx = 0; domain_idx < M_NUM_CORE; ++domain_idx) {
            m_core_scal.push_back({m_platform_io.push_signal("MSR::CPU_SCALABILITY_RATIO",
                                                             GEOPM_DOMAIN_CORE,
                                                             domain_idx), NAN});
            m_core_freq_control.push_back({m_platform_io.push_control("CPU_FREQUENCY_CONTROL",
                                                                      GEOPM_DOMAIN_CORE,
                                                                      domain_idx), -1});
        }

        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            m_qm_rate.push_back({m_platform_io.push_signal("QM_CTR_SCALED_RATE",
                                                           GEOPM_DOMAIN_PACKAGE,
                                                           domain_idx), NAN});

            m_uncore_freq_status.push_back({m_platform_io.push_signal("MSR::UNCORE_PERF_STATUS:FREQ",
                                                                      GEOPM_DOMAIN_PACKAGE,
                                                                      domain_idx), NAN});
        }

        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            m_uncore_freq_min_control.push_back({m_platform_io.push_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO",
                                              GEOPM_DOMAIN_PACKAGE,
                                              domain_idx), -1});
            m_uncore_freq_max_control.push_back({m_platform_io.push_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO",
                                              GEOPM_DOMAIN_PACKAGE,
                                              domain_idx), -1});
        }

        //Configuration of QM_CTR must match QM_CTR config used for training data
        m_platform_io.write_control("MSR::PQR_ASSOC:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
        m_platform_io.write_control("MSR::QM_EVTSEL:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
        m_platform_io.write_control("MSR::QM_EVTSEL:EVENT_ID", GEOPM_DOMAIN_BOARD, 0, 2);
    }

    // Validate incoming policy and configure default policy requests.
    void CPUActivityAgent::validate_policy(std::vector<double> &in_policy) const
    {
        assert(in_policy.size() == M_NUM_POLICY);
        ///////////////////////
        //CPU POLICY CHECKING//
        ///////////////////////
        double freq_core_min = m_platform_io.read_signal("CPU_FREQUENCY_MIN", GEOPM_DOMAIN_BOARD, 0);
        double freq_core_max = m_platform_io.read_signal("CPU_FREQUENCY_MAX", GEOPM_DOMAIN_BOARD, 0);

        // Check for NAN to set default values for policy
        if (std::isnan(in_policy[M_POLICY_CPU_FREQ_MAX])) {
            in_policy[M_POLICY_CPU_FREQ_MAX] = freq_core_max;
        }

        if (in_policy[M_POLICY_CPU_FREQ_MAX] > freq_core_max ||
            in_policy[M_POLICY_CPU_FREQ_MAX] < freq_core_min ) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "():CPU_FREQ_MAX out of range: " +
                            std::to_string(in_policy[M_POLICY_CPU_FREQ_MAX]) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Check for NAN to set default values for policy
        if (std::isnan(in_policy[M_POLICY_CPU_FREQ_EFFICIENT])) {
            in_policy[M_POLICY_CPU_FREQ_EFFICIENT] = freq_core_min;
        }

        if (in_policy[M_POLICY_CPU_FREQ_EFFICIENT] > freq_core_max ||
            in_policy[M_POLICY_CPU_FREQ_EFFICIENT] < freq_core_min ) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "():CPU_FREQ_EFFICIENT out of range: " +
                            std::to_string(in_policy[M_POLICY_CPU_FREQ_EFFICIENT]) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (in_policy[M_POLICY_CPU_FREQ_EFFICIENT] > in_policy[M_POLICY_CPU_FREQ_MAX]) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "():CPU_FREQ_EFFICIENT (" +
                            std::to_string(in_policy[M_POLICY_CPU_FREQ_EFFICIENT]) +
                            ") value exceeds CPU_FREQ_MAX (" +
                            std::to_string(in_policy[M_POLICY_CPU_FREQ_MAX]) +
                            ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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

        if (in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT] > in_policy[M_POLICY_UNCORE_FREQ_MAX]) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "():UNCORE_FREQ_EFFICIENT (" +
                            std::to_string(in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT]) +
                            ") value exceeds UNCORE_FREQ_MAX (" +
                            std::to_string(in_policy[M_POLICY_UNCORE_FREQ_MAX]) +
                            ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // If no sample period is provided assume the default behavior
        if (std::isnan(in_policy[M_POLICY_SAMPLE_PERIOD])) {
            in_policy[M_POLICY_SAMPLE_PERIOD] = M_WAIT_SEC;
        }

        if (!std::isnan(in_policy[M_POLICY_SAMPLE_PERIOD])) {
            if (in_policy[M_POLICY_SAMPLE_PERIOD] <= 0.0) {
                throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                "(): Sample period must be greater than 0.",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);

            }
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

    }

    // Distribute incoming policy to children
    void CPUActivityAgent::split_policy(const std::vector<double>& in_policy,
                                    std::vector<std::vector<double> >& out_policy)
    {
        assert(in_policy.size() == M_NUM_POLICY);
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool CPUActivityAgent::do_send_policy(void) const
    {
        return true;
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
        assert(in_policy.size() == M_NUM_POLICY);

        m_do_write_batch = false;

        // Per core freq
        std::vector<double> core_freq_request;

        // Per package freq
        std::vector<double> uncore_freq_request;
        double f_core_efficient = in_policy[M_POLICY_CPU_FREQ_EFFICIENT];
        double f_core_range = in_policy[M_POLICY_CPU_FREQ_MAX] - in_policy[M_POLICY_CPU_FREQ_EFFICIENT];

        double f_uncore_efficient = in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT];
        double f_uncore_range = in_policy[M_POLICY_UNCORE_FREQ_MAX] - in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT];

        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            double uncore_freq = (double) m_uncore_freq_status.at(domain_idx).signal;

            /////////////////////////////////////////////
            // L3 Total External Bandwidth Measurement //
            /////////////////////////////////////////////
            auto qm_max_itr = m_qm_max_rate.lower_bound(uncore_freq);
            if(qm_max_itr != m_qm_max_rate.begin()) {
                qm_max_itr = std::prev(qm_max_itr, 1);
            }
            double qm_normalized = (double) m_qm_rate.at(domain_idx).signal /
                                            qm_max_itr->second;

            // L3 usage, Network Traffic, HBM, and PCIE (GPUs) all use the uncore.
            // Eventually all these components should be considered when scaling
            // the uncore frequency in the efficient - performant range.
            // A more robust/future proof solution may be to directly query uncore
            // counters that indicate utilization (when/if available).
            // For now only L3 bandwith metric is used.
            double scalability_uncore = qm_normalized;
            double uncore_req = f_uncore_efficient + f_uncore_range * scalability_uncore;

            //Clip uncore request within policy limits
            uncore_req = std::max(in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT], uncore_req);
            uncore_req = std::min(in_policy[M_POLICY_UNCORE_FREQ_MAX], uncore_req);
            uncore_freq_request.push_back(uncore_req);
        }

        for (int domain_idx = 0; domain_idx < M_NUM_CORE; ++domain_idx) {
            //////////////////////////////////
            // Core Scalability Measurement //
            //////////////////////////////////
            double scalability = (double) m_core_scal.at(domain_idx).signal;
            if (std::isnan(scalability)) {
                scalability = 1.0;
            }

            double core_req = f_core_efficient + f_core_range * scalability;
            //Clip core request within policy limits
            core_req = std::max(in_policy[M_POLICY_CPU_FREQ_EFFICIENT], core_req);
            core_req = std::min(in_policy[M_POLICY_CPU_FREQ_MAX], core_req);
            core_freq_request.push_back(core_req);
        }

        for (int domain_idx = 0; domain_idx < M_NUM_CORE; ++domain_idx) {
            if(std::isnan(core_freq_request.at(domain_idx))) {
                core_freq_request.at(domain_idx) = in_policy[M_POLICY_CPU_FREQ_MAX];
            }
            if (core_freq_request.at(domain_idx) !=
                m_core_freq_control.at(domain_idx).last_setting) {

                m_platform_io.adjust(m_core_freq_control.at(domain_idx).batch_idx,
                                     core_freq_request.at(domain_idx));

                m_core_freq_control.at(domain_idx).last_setting = core_freq_request.at(domain_idx);
                ++m_core_frequency_requests;
                m_do_write_batch = true;
            }
        }

        // set frequency control per package
        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            if(std::isnan(uncore_freq_request.at(domain_idx))) {
                uncore_freq_request.at(domain_idx) = in_policy[M_POLICY_UNCORE_FREQ_MAX];
            }

            if (uncore_freq_request.at(domain_idx) !=
                m_uncore_freq_min_control.at(domain_idx).last_setting ||
                uncore_freq_request.at(domain_idx) !=
                m_uncore_freq_max_control.at(domain_idx).last_setting) {
                //Adjust
                m_platform_io.adjust(m_uncore_freq_min_control.at(domain_idx).batch_idx,
                                    uncore_freq_request.at(domain_idx));

                m_platform_io.adjust(m_uncore_freq_max_control.at(domain_idx).batch_idx,
                                    uncore_freq_request.at(domain_idx));

                //save the value for future comparison
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
        return m_do_write_batch;
    }

    // Read signals from the platform and calculate samples to be sent up
    void CPUActivityAgent::sample_platform(std::vector<double> &out_sample)
    {
        assert(out_sample.size() == M_NUM_SAMPLE);

        // Collect latest signal values
        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            // Frequency signals
            m_uncore_freq_status.at(domain_idx).signal = m_platform_io.sample(m_uncore_freq_status.at(domain_idx).batch_idx);

            // Uncore steering signals
            m_qm_rate.at(domain_idx).signal = m_platform_io.sample(m_qm_rate.at(domain_idx).batch_idx);
        }

        for (int domain_idx = 0; domain_idx < M_NUM_CORE; ++domain_idx) {
            // Core steering signals
            m_core_scal.at(domain_idx).signal = m_platform_io.sample(m_core_scal.at(domain_idx).batch_idx);
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
        result.push_back({"Uncore Frequency Requests", std::to_string(m_uncore_frequency_requests)});
        result.push_back({"Network Normalized Frequency Requests", std::to_string(m_network_normalized_frequency_requests)});
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
        return {"CPU_FREQ_MAX", "CPU_FREQ_EFFICIENT", "UNCORE_FREQ_MAX", "UNCORE_FREQ_EFFICIENT", "CPU_PHI", "SAMPLE_PERIOD"};
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> CPUActivityAgent::sample_names(void)
    {
        return {};
    }
}
