/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CPUActivityAgent.hpp"

#include <cmath>
#include <cassert>
#include <algorithm>
#include <iostream>

#include "geopm/PlatformIOProf.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

#include <string>

using geopm::Agent;
using geopm::PlatformIO;
using geopm::PlatformTopo;

// Registers this Agent with the Agent factory, making it visible
// to the Controller when the plugin is first loaded.
static void __attribute__((constructor)) torch_agent_load(void)
{
    geopm::agent_factory().register_plugin(CPUActivityAgent::plugin_name(),
                                           CPUActivityAgent::make_plugin,
                                           Agent::make_dictionary(CPUActivityAgent::policy_names(),
                                                                  CPUActivityAgent::sample_names()));
}


CPUActivityAgent::CPUActivityAgent()
    : CPUActivityAgent(geopm::platform_io(), geopm::platform_topo())
{

}

CPUActivityAgent::CPUActivityAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
    : m_platform_io(plat_io)
    , m_platform_topo(topo)
    , m_last_wait{{0, 0}}
    , M_WAIT_SEC(0.010) // 10ms Wait
    , M_POLICY_PHI_DEFAULT(0.5)
    , M_NUM_PACKAGE(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
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
    m_frequency_requests = 0;
    m_uncore_frequency_requests = 0;

    init_platform_io();
}

void CPUActivityAgent::init_platform_io(void)
{
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_freq_status.push_back({m_platform_io.push_signal("CPU_FREQUENCY_STATUS",
                                         GEOPM_DOMAIN_PACKAGE,
                                         domain_idx), NAN});
        m_uncore_freq_status.push_back({m_platform_io.push_signal("MSR::UNCORE_PERF_STATUS:FREQ",
                                                GEOPM_DOMAIN_PACKAGE,
                                                domain_idx), NAN});
        m_qm_rate.push_back({m_platform_io.push_signal("QM_CTR_SCALED_RATE",
                                     GEOPM_DOMAIN_PACKAGE,
                                     domain_idx), NAN});
        m_inst_retired.push_back({m_platform_io.push_signal("INSTRUCTIONS_RETIRED",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), NAN});
        m_cycles_unhalted.push_back({m_platform_io.push_signal("CYCLES_THREAD",
                                             GEOPM_DOMAIN_PACKAGE,
                                             domain_idx), NAN});
        m_scal.push_back({m_platform_io.push_signal("MSR::CPU_SCALABILITY_RATIO",
                                  GEOPM_DOMAIN_PACKAGE,
                                  domain_idx), NAN});
    }

    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_core_freq_control.push_back({m_platform_io.push_control("CPU_FREQUENCY_CONTROL",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), -1});

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
    double min_freq = m_platform_io.read_signal("CPU_FREQUENCY_MIN", GEOPM_DOMAIN_BOARD, 0);
    double max_freq = m_platform_io.read_signal("CPU_FREQUENCY_MAX", GEOPM_DOMAIN_BOARD, 0);

    ///////////////////////
    //CPU POLICY CHECKING//
    ///////////////////////
    // Check for NAN to set default values for policy
    if (std::isnan(in_policy[M_POLICY_CORE_FREQ_MAX])) {
        in_policy[M_POLICY_CORE_FREQ_MAX] = max_freq;
    }

    if (in_policy[M_POLICY_CORE_FREQ_MAX] > max_freq ||
        in_policy[M_POLICY_CORE_FREQ_MAX] < min_freq ) {
        throw geopm::Exception("CPUActivityAgent::" + std::string(__func__) +
                        "():_FREQ_MAX out of range: " +
                        std::to_string(in_policy[M_POLICY_CORE_FREQ_MAX]) +
                        ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    // Check for NAN to set default values for policy
    if (std::isnan(in_policy[M_POLICY_CORE_FREQ_MIN])) {
        in_policy[M_POLICY_CORE_FREQ_MIN] = min_freq;
    }

    if (in_policy[M_POLICY_CORE_FREQ_MIN] > max_freq ||
        in_policy[M_POLICY_CORE_FREQ_MIN] < min_freq ) {
        throw geopm::Exception("CPUActivityAgent::" + std::string(__func__) +
                        "():_FREQ_MIN out of range: " +
                        std::to_string(in_policy[M_POLICY_CORE_FREQ_MIN]) +
                        ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    if (in_policy[M_POLICY_CORE_FREQ_MIN] > in_policy[M_POLICY_CORE_FREQ_MAX]) {
        throw geopm::Exception("CPUActivityAgent::" + std::string(__func__) +
                        "():_FREQ_MIN (" +
                        std::to_string(in_policy[M_POLICY_CORE_FREQ_MIN]) +
                        ") value exceeds_FREQ_MAX (" +
                        std::to_string(in_policy[M_POLICY_CORE_FREQ_MAX]) +
                        ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    // If no phi value is provided assume the default behavior.
    //if (std::isnan(in_policy[M_POLICY_CPU_PHI])) {
    //    in_policy[M_POLICY_CPU_PHI] = M_POLICY_PHI_DEFAULT;
    //}

    //if (in_policy[M_POLICY_CPU_PHI] < 0.0 ||
    //    in_policy[M_POLICY_CPU_PHI] > 1.0) {
    //    throw geopm::Exception("CPUActivityAgent::" + std::string(__func__) +
    //                           "(): POLICY_CPU_PHI value out of range: " +
    //                           std::to_string(in_policy[M_POLICY_CPU_PHI]) + ".",
    //                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    //}
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

    // Per freq
    std::vector<double> core_freq_request;
    std::vector<double> uncore_freq_request;
    double core_fe = in_policy[M_POLICY_CORE_FREQ_MIN];
    double core_range = in_policy[M_POLICY_CORE_FREQ_MAX] - in_policy[M_POLICY_CORE_FREQ_MIN];

    double uncore_fe = in_policy[M_POLICY_UNCORE_FREQ_MIN];
    double uncore_range = in_policy[M_POLICY_UNCORE_FREQ_MAX] - in_policy[M_POLICY_UNCORE_FREQ_MIN];

    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        double uncore_freq = (double) m_uncore_freq_status.at(domain_idx).signal;

        //Lower bound is not ideal here.  We want the lower bound - 1
        auto qm_max_itr = m_qm_max_rate.lower_bound(uncore_freq);
        if(qm_max_itr != m_qm_max_rate.begin()) {
            qm_max_itr = std::prev(qm_max_itr, 1);
        }
        double qm_normalized = (double) m_qm_rate.at(domain_idx).signal /
                                        qm_max_itr->second;

        double ipc = (double) m_inst_retired.at(domain_idx).sample /
                              m_cycles_unhalted.at(domain_idx).sample;

        double scalability = (double) m_scal.at(domain_idx).signal;
        if (std::isnan(scalability)) {
            scalability = 1.0;
        }

        double core_req = core_fe + core_range * scalability;
        //double core_req = core_fe + core_range * (ipc / 4); //TODO: formalize an approach for ipc normalization
                                                              //      if scalability isn't available
        double uncore_req = uncore_fe + uncore_range * qm_normalized;

        core_freq_request.push_back(core_req);
        uncore_freq_request.push_back(uncore_req);
    }

    // set frequency control per package
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        if(std::isnan(core_freq_request.at(domain_idx))) {
            core_freq_request.at(domain_idx) = in_policy[M_POLICY_CORE_FREQ_MAX];
        }
        if(std::isnan(uncore_freq_request.at(domain_idx))) {
            uncore_freq_request.at(domain_idx) = in_policy[M_POLICY_UNCORE_FREQ_MAX];
        }

        if (core_freq_request.at(domain_idx) !=
            m_core_freq_control.at(domain_idx).last_setting ||
            uncore_freq_request.at(domain_idx) !=
            m_uncore_freq_min_control.at(domain_idx).last_setting ||
            uncore_freq_request.at(domain_idx) !=
            m_uncore_freq_max_control.at(domain_idx).last_setting) {
            //Adjust
            m_platform_io.adjust(m_core_freq_control.at(domain_idx).batch_idx,
                                core_freq_request.at(domain_idx));

            m_platform_io.adjust(m_uncore_freq_min_control.at(domain_idx).batch_idx,
                                uncore_freq_request.at(domain_idx));

            m_platform_io.adjust(m_uncore_freq_max_control.at(domain_idx).batch_idx,
                                uncore_freq_request.at(domain_idx));

            //save the value for future comparison
            m_core_freq_control.at(domain_idx).last_setting = core_freq_request.at(domain_idx);
            m_uncore_freq_min_control.at(domain_idx).last_setting = uncore_freq_request.at(domain_idx);
            m_uncore_freq_max_control.at(domain_idx).last_setting = uncore_freq_request.at(domain_idx);

            ++m_frequency_requests;
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
        m_freq_status.at(domain_idx).signal = m_platform_io.sample(m_freq_status.at(domain_idx).batch_idx);
        m_uncore_freq_status.at(domain_idx).signal = m_platform_io.sample(m_uncore_freq_status.at(domain_idx).batch_idx);
        m_qm_rate.at(domain_idx).signal = m_platform_io.sample(m_qm_rate.at(domain_idx).batch_idx);

        //Clk unhalted cycles diff and new value
        m_cycles_unhalted.at(domain_idx).sample = m_platform_io.sample(m_cycles_unhalted.at(domain_idx).batch_idx) -
                                                          m_cycles_unhalted.at(domain_idx).signal;
        m_cycles_unhalted.at(domain_idx).signal = m_platform_io.sample(m_cycles_unhalted.at(domain_idx).batch_idx);

        //Inst Retired diff and new value
        m_inst_retired.at(domain_idx).sample = m_platform_io.sample(m_inst_retired.at(domain_idx).batch_idx) -
                                                       m_inst_retired.at(domain_idx).signal;
        m_inst_retired.at(domain_idx).signal = m_platform_io.sample(m_inst_retired.at(domain_idx).batch_idx);

        m_scal.at(domain_idx).signal = m_platform_io.sample(m_scal.at(domain_idx).batch_idx);
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

    result.push_back({"Xeon Package Frequency Requests", std::to_string(m_frequency_requests)});
    result.push_back({"Xeon Uncore Frequency Requests", std::to_string(m_uncore_frequency_requests)});
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
    return {"CORE_FREQ_MIN", "CORE_FREQ_MAX", "UNCORE_FREQ_MIN", "UNCORE_FREQ_MAX"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> CPUActivityAgent::sample_names(void)
{
    return {};
}
