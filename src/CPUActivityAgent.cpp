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
#include "geopm_debug.hpp"

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
        , m_update_characterization(true)
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void CPUActivityAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_uncore_frequency_requests = 0;
        m_f_uncore_efficient = 0;
        m_f_uncore_max = 0;
        m_f_uncore_range = 0;
        m_resolved_f_uncore_efficient = 0;
        m_resolved_f_uncore_max = 0;
        m_resolved_f_uncore_range = 0;

        m_core_frequency_requests = 0;
        m_f_core_efficient = 0;
        m_f_core_max = 0;
        m_f_core_range = 0;
        m_resolved_f_core_efficient = 0;
        m_resolved_f_core_max = 0;
        m_resolved_f_core_range = 0;

        // These are not currently guaranteed to be the system uncore min and max,
        // just what the user/admin has previously set.
        m_freq_uncore_min = m_platform_io.read_signal("CPU_UNCORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0);
        m_freq_uncore_max = m_platform_io.read_signal("CPU_UNCORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0);

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
            m_uncore_freq_min_control.push_back({m_platform_io.push_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL",
                                              GEOPM_DOMAIN_PACKAGE,
                                              domain_idx), -1});
            m_uncore_freq_max_control.push_back({m_platform_io.push_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL",
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
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY,
                           "CPUActivityAgent::" + std::string(__func__) +
                           "(): policy vector not correctly sized.  Expected  " +
                            std::to_string(M_NUM_POLICY) + ", actual: " +
                            std::to_string(in_policy.size()));

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

        // Validate all (uncore frequency, max memory bandwidth) pairs
        std::set<double> policy_uncore_freqs;
        for (auto it = in_policy.begin() + M_POLICY_FIRST_UNCORE_FREQ;
             it != in_policy.end() && std::next(it) != in_policy.end();
             std::advance(it, 2)) {

            auto mapped_mem_bw = *(it + 1);
            if (!std::isnan(*it)) {
                // Not valid to have NAN max mem bw for uncore freq.
                auto uncore_freq = static_cast<uint64_t>(*it);
                if (std::isnan(mapped_mem_bw)) {
                    throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                    "(): mapped CPU_UNCORE_FREQUENCY with no max memory bandwidth assigned.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
                // Make sure the frequency does not have multiple definitions.
                if (!policy_uncore_freqs.insert(uncore_freq).second) {
                    throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                    " policy has multiple entries for CPU_UNCORE_FREQUENCY " +
                                    std::to_string(uncore_freq),
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            else if (!std::isnan(mapped_mem_bw)) {
                throw Exception("CPUActivityAgent::" + std::string(__func__) +
                                " policy maps a NAN CPU_UNCORE_FREQUENCY with max memory bandwidth: " +
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

    // Handle per node characterization settings and policy overrides
    void CPUActivityAgent::node_characterization(const std::vector<double>& in_policy) {
        ///////////////////////////
        // Core characterization //
        ///////////////////////////
        auto all_names = m_platform_io.signal_names();
        double freq_core_min = m_platform_io.read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
        double freq_core_max = m_platform_io.read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

        // Max Core Frequency Selection
        if (std::isnan(in_policy[M_POLICY_CPU_FREQ_MAX])) {
            // If no value is provided use the local node setting
            m_f_core_max = freq_core_max;
        } else {
            m_f_core_max = in_policy[M_POLICY_CPU_FREQ_MAX];
        }

        if (m_f_core_max > freq_core_max ||
            m_f_core_max < freq_core_min) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): Core maximum frequency out of system range before applying PHI: " +
                            std::to_string(m_f_core_max) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        // Efficient Core Frequency Selection
        if (std::isnan(in_policy[M_POLICY_CPU_FREQ_EFFICIENT])) {
            // If no policy value is provided we use the
            // node local characterization value or the
            // node local minimum value
            if (all_names.find("NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT") != all_names.end()) {
                m_f_core_efficient = m_platform_io.read_signal("NODE_CHARACTERIZATION::CPU_CORE_FREQUENCY_EFFICIENT",
                                                                          GEOPM_DOMAIN_BOARD, 0);
            }
            // If the characterization signal is not present or
            // is otherwise non-sensical use the minimum core frequency
            if (std::isnan(m_f_core_efficient) ||
                m_f_core_efficient == 0) {
                m_f_core_efficient = freq_core_min;
            }
        }
        else {
            // If the policy provides an non-NAN efficient frequency it will
            // override any node local settings and be used for all nodes
            m_f_core_efficient = in_policy[M_POLICY_CPU_FREQ_EFFICIENT];
        }

        // Determing default range
        m_f_core_range = m_f_core_max - m_f_core_efficient;

        if (m_f_core_efficient > freq_core_max ||
            m_f_core_efficient < freq_core_min ) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): Core efficient frequency out of system range before applying PHI: " +
                            std::to_string(m_f_core_efficient) +
                            ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (m_f_core_efficient > m_f_core_max) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): Core efficient frequency (" +
                            std::to_string(m_f_core_efficient) +
                            ") value exceeds core max frequency (" +
                            std::to_string(m_f_core_max) +
                            ") before applying PHI.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        /////////////////////////////
        // Uncore characterization //
        /////////////////////////////

        // Max Uncore Frequency Selection
        if (std::isnan(in_policy[M_POLICY_UNCORE_FREQ_MAX])) {
            // If no value is provided use the local node setting
            m_f_uncore_max = m_freq_uncore_max;
        } else {
            m_f_uncore_max = in_policy[M_POLICY_UNCORE_FREQ_MAX];
        }

        // Efficient Uncore Frequency Selection
        if (std::isnan(in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT])) {
            // If no policy value is provided we use the
            // node local characterization value or the
            // node local minimum value
            if (all_names.find("NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT") != all_names.end()) {
                m_f_uncore_efficient = m_platform_io.read_signal("NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_EFFICIENT",
                                                                          GEOPM_DOMAIN_BOARD, 0);
            }
            // If the characterization signal is not present or
            // is otherwise non-sensical use the minimum uncore frequency
            if (std::isnan(m_f_uncore_efficient) ||
                m_f_uncore_efficient == 0) {
                m_f_uncore_efficient = m_freq_uncore_min;
            }
        }
        else {
            // If the policy provides an non-NAN efficient frequency it will
            // override any node local settings and be used for all nodes
            m_f_uncore_efficient = in_policy[M_POLICY_UNCORE_FREQ_EFFICIENT];
        }

        // Determing default range
        m_f_core_range = m_f_core_max - m_f_core_efficient;
        m_f_uncore_range = m_f_uncore_max - m_f_uncore_efficient;

        if (m_f_uncore_efficient > m_f_uncore_max) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): Uncore efficient frequency (" +
                            std::to_string(m_f_uncore_efficient) +
                            ") value exceeds Uncore max frequency (" +
                            std::to_string(m_f_uncore_max) +
                            ") before applying PHI.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        ////////////////////////////////////
        // Revaluate frequencies with PHI //
        ////////////////////////////////////
        double phi = in_policy[M_POLICY_CPU_PHI];

        // The default (phi == 0.5) behavior is the frequencies calculated above
        m_resolved_f_core_max = m_f_core_max;
        m_resolved_f_core_efficient = m_f_core_efficient;
        m_resolved_f_uncore_max = m_f_uncore_max;
        m_resolved_f_uncore_efficient = m_f_uncore_efficient;

        // If phi is not 0.5 we move into the energy or performance biased behavior
        if (phi > 0.5) {
            // Energy Biased.  Scale F_max down to F_efficient based upon phi value
            // Active region phi usage
            m_resolved_f_core_max = std::max(m_f_core_efficient,
                                             m_f_core_max -
                                             m_f_core_range * (phi-0.5) / 0.5);

            m_resolved_f_uncore_max = std::max(m_f_uncore_efficient,
                                               m_f_uncore_max -
                                               m_f_uncore_range * (phi-0.5) / 0.5);
        }
        else if (phi < 0.5) {
            // Perf Biased.  Scale F_efficient up to F_max based upon phi value
            // Active region phi usage
            m_resolved_f_core_efficient = std::min(m_f_core_max,
                                                   m_f_core_efficient +
                                                   m_f_core_range * (0.5-phi) / 0.5);

            m_resolved_f_uncore_efficient = std::min(m_f_uncore_max,
                                                     m_f_uncore_efficient +
                                                     m_f_uncore_range * (0.5-phi) / 0.5);
        }

        m_resolved_f_core_range = m_resolved_f_core_max - m_resolved_f_core_efficient;
        m_resolved_f_uncore_range = m_resolved_f_uncore_max - m_resolved_f_uncore_efficient;

        if (m_resolved_f_uncore_efficient > m_resolved_f_uncore_max) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): Resolved uncore efficient frequency (" +
                            std::to_string(m_resolved_f_uncore_efficient) +
                            ") value exceeds resolved uncore max frequency (" +
                            std::to_string(m_resolved_f_uncore_max) +
                            ") after applying PHI.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (m_resolved_f_core_efficient > m_resolved_f_core_max) {
            throw Exception("CPUActivityAgent::" + std::string(__func__) +
                            "(): Resolved core efficient frequency (" +
                            std::to_string(m_resolved_f_core_efficient) +
                            ") value exceeds resolved core max frequency (" +
                            std::to_string(m_resolved_f_core_max) +
                            ") after applying PHI.", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        ///////////////////////////////////////////
        // Max memory bandwidth characterization //
        ///////////////////////////////////////////
        for (auto it = in_policy.begin() + M_POLICY_FIRST_UNCORE_FREQ;
             it != in_policy.end() && std::next(it) != in_policy.end();
             std::advance(it, 2)) {

            if (!std::isnan(*it)) {
                auto uncore_freq = static_cast<uint64_t>(*it);
                auto max_mem_bw = *(it + 1);
                // Not valid to have NAN max mem bw for uncore freq.
                GEOPM_DEBUG_ASSERT(!std::isnan(max_mem_bw),
                                   "mapped CPU_UNCORE_FREQUENCY with no max memory bandwidth assigned.");
                // This is handled here as the validate_policy is const.
                // The extra assertion is likely unnecessary as the validate_policy
                // check should catch this case.
                m_qm_max_rate[uncore_freq] = max_mem_bw;
            }
        }
        // If no policy value is provided we use the
        // node local characterization value or the
        // node local minimum value
        if (m_qm_max_rate.size() == 0) {
            // We do not guarantee an ordering or limit to the MBM characterization entries,
            // so we check all characterization entries to see if they are MBM characterization
            for (int entry_idx = 0; entry_idx < (int)all_names.size(); ++entry_idx) {
                std::string key_name = "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_" +
                                       std::to_string(entry_idx);
                std::string val_name = "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_" +
                                       std::to_string(entry_idx);
                if (all_names.find(key_name) != all_names.end() &&
                    all_names.find(val_name) != all_names.end()) {
                    double uncore_freq = m_platform_io.read_signal(key_name, GEOPM_DOMAIN_BOARD, 0);
                    double max_mem_bw = m_platform_io.read_signal(val_name, GEOPM_DOMAIN_BOARD, 0);
                    if (!std::isnan(uncore_freq) && uncore_freq != 0 &&
                        max_mem_bw != 0) {
                        m_qm_max_rate[uncore_freq] = max_mem_bw;
                    }
                }
            }

            // Warn the user if they have not entered a policy with memory bandwidth characterization
            // and there was no per-node configuration available for the agent to use.  In this case
            // the phi-adjusted maximum uncore frequency will be used.
            if (m_qm_max_rate.size() == 0) {
                std::cerr << "Warning: <geopm> CPUActivityAgent did not receive a policy containing memory "
                             "bandwidth characterization and failed to locate the "
                             "NODE_CHARACTERIZATION::CPU_UNCORE_FREQUENCY_#, or "
                             "NODE_CHARACTERIZATION::CPU_UNCORE_MAXIMUM_MEMORY_BANDWIDTH_# signals.  This will "
                             "disable dynamic uncore frequency selection and may negatively impact agent "
                             "performance." << std::endl;
            }
        }
    }

    void CPUActivityAgent::adjust_platform(const std::vector<double>& in_policy)
    {
        m_do_write_batch = false;

        if (m_update_characterization) {
            node_characterization(in_policy);
            m_update_characterization = false;
        }

        // Per package freq
        std::vector<double> uncore_freq_request;

        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            double uncore_freq = (double) m_uncore_freq_status.at(domain_idx).signal;

            /////////////////////////////////////////////
            // L3 Total External Bandwidth Measurement //
            /////////////////////////////////////////////
            auto qm_max_itr = m_qm_max_rate.lower_bound(uncore_freq);
            if(qm_max_itr != m_qm_max_rate.begin()) {
                qm_max_itr = std::prev(qm_max_itr, 1);
            }

            double qm_normalized = 1.0;

            // Handle divided by zero, either numerator or
            // denominator being NAN, and the un-characterized case
            if (!std::isnan(m_qm_rate.at(domain_idx).signal) &&
                !std::isnan(qm_max_itr->second) &&
                qm_max_itr->second != 0 &&
                m_qm_max_rate.size() != 0) {
                qm_normalized = (double) m_qm_rate.at(domain_idx).signal /
                                         qm_max_itr->second;
            }

            // L3 usage, Network Traffic, HBM, and PCIE (GPUs) all use the uncore.
            // Eventually all these components should be considered when scaling
            // the uncore frequency in the efficient - performant range.
            // A more robust/future proof solution may be to directly query uncore
            // counters that indicate utilization (when/if available).
            // For now only L3 bandwith metric is used.
            double scalability_uncore = qm_normalized;
            double uncore_req = m_resolved_f_uncore_efficient + m_resolved_f_uncore_range * scalability_uncore;

            //Clip uncore request within policy limits
            uncore_req = std::max(m_resolved_f_uncore_efficient, uncore_req);
            uncore_req = std::min(m_resolved_f_uncore_max, uncore_req);
            uncore_freq_request.push_back(uncore_req);
        }

        // Per core freq
        std::vector<double> core_freq_request;

        for (int domain_idx = 0; domain_idx < M_NUM_CORE; ++domain_idx) {
            //////////////////////////////////
            // Core Scalability Measurement //
            //////////////////////////////////
            double scalability = (double) m_core_scal.at(domain_idx).signal;
            if (std::isnan(scalability)) {
                scalability = 1.0;
            }

            double core_req = m_resolved_f_core_efficient + m_resolved_f_core_range * scalability;

            // Clip core request within reasonable limits
            // Never request below the efficient frequency
            core_req = std::max(m_resolved_f_core_efficient, core_req);
            // Never request above the maximum frequency
            core_req = std::min(m_resolved_f_core_max, core_req);
            core_freq_request.push_back(core_req);
        }

        // set per core controls
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

        // set per package controls
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

        result.push_back({"Initial (Pre-PHI) Maximum Core Frequency", std::to_string(m_f_core_max)});
        result.push_back({"Initial (Pre-PHI) Efficient Core Frequency", std::to_string(m_f_core_efficient)});
        result.push_back({"Initial (Pre-PHI) Core Frequency Range", std::to_string(m_f_core_range)});
        result.push_back({"Initial (Pre-PHI) Maximum Uncore Frequency", std::to_string(m_f_uncore_max)});
        result.push_back({"Initial (Pre-PHI) Efficient Uncore Frequency", std::to_string(m_f_uncore_efficient)});
        result.push_back({"Initial (Pre-PHI) Uncore Frequency Range", std::to_string(m_f_uncore_range)});

        result.push_back({"Actual (Post-PHI) Maximum Core Frequency", std::to_string(m_resolved_f_core_max)});
        result.push_back({"Actual (Post-PHI) Efficient Core Frequency", std::to_string(m_resolved_f_core_efficient)});
        result.push_back({"Actual (Post-PHI) Core Frequency Range", std::to_string(m_resolved_f_core_range)});
        result.push_back({"Actual (Post-PHI) Maximum Uncore Frequency", std::to_string(m_resolved_f_uncore_max)});
        result.push_back({"Actual (Post-PHI) Efficient Uncore Frequency", std::to_string(m_resolved_f_uncore_efficient)});
        result.push_back({"Actual (Post-PHI) Uncore Frequency Range", std::to_string(m_resolved_f_uncore_range)});

        for(auto uncore_mbm_kv : m_qm_max_rate) {
            result.push_back({"Uncore Frequency " + std::to_string(uncore_mbm_kv.first) +
                              " Maximum Memory Bandwidth", std::to_string(uncore_mbm_kv.second)});
        }
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
        std::vector<std::string> names{"CPU_FREQ_MAX", "CPU_FREQ_EFFICIENT", "CPU_UNCORE_FREQ_MAX",
                                       "CPU_UNCORE_FREQ_EFFICIENT", "CPU_PHI", "SAMPLE_PERIOD"};
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
