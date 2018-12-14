/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <cmath>
#include <algorithm>
#include <assert.h>

#include "ExampleAgent.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"

using geopm::Agent;
using geopm::IPlatformIO;
using geopm::IPlatformTopo;

// Registers this Agent with the Agent factory, making it visible
// to the Controller when the plugin is first loaded.
static void __attribute__((constructor)) example_agent_load(void)
{
    geopm::agent_factory().register_plugin(ExampleAgent::plugin_name(),
                                           ExampleAgent::make_plugin,
                                           Agent::make_dictionary(ExampleAgent::policy_names(),
                                                                  ExampleAgent::sample_names()));
}

ExampleAgent::ExampleAgent()
    : m_platform_io(geopm::platform_io())
    , m_platform_topo(geopm::platform_topo())
    , m_signal_idx(M_NUM_PLAT_SIGNAL, -1)
    , m_control_idx(M_NUM_PLAT_CONTROL, -1)
    , m_last_sample(M_NUM_SAMPLE, NAN)
    , m_last_signal(M_NUM_PLAT_SIGNAL, NAN)
    , m_last_wait{{0, 0}}
    , M_WAIT_SEC(1.0)
    , m_min_idle(NAN)
    , m_max_idle(NAN)
{
    geopm_time(&m_last_wait);
}

// Push signals and controls for future batch read/write
void ExampleAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    // all signals and controls will be at board domain
    int board = IPlatformTopo::M_DOMAIN_BOARD;
    // push signals
    m_signal_idx[M_PLAT_SIGNAL_USER] = m_platform_io.push_signal("USER_TIME", board, 0);
    m_signal_idx[M_PLAT_SIGNAL_SYSTEM] = m_platform_io.push_signal("SYSTEM_TIME", board, 0);
    m_signal_idx[M_PLAT_SIGNAL_IDLE] = m_platform_io.push_signal("IDLE_TIME", board, 0);
    m_signal_idx[M_PLAT_SIGNAL_NICE] = m_platform_io.push_signal("NICE_TIME", board, 0);
    // push controls
    m_control_idx[M_PLAT_CONTROL_STDOUT] = m_platform_io.push_control("STDOUT", board, 0);
    m_control_idx[M_PLAT_CONTROL_STDERR] = m_platform_io.push_control("STDERR", board, 0);
}

// Validate incoming policy and configure default policy requests.
std::vector<double> ExampleAgent::set_policy_defaults(const std::vector<double> &in_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    return in_policy;
}

// Distribute incoming policy to children
bool ExampleAgent::descend(const std::vector<double> &in_policy,
                           std::vector<std::vector<double> >&out_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
    return true;
}

// Aggregate average utilization samples from children
bool ExampleAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                          std::vector<double> &out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);
    std::vector<double> child_sample(in_sample.size());
    for (size_t sample_idx = 0; sample_idx < M_NUM_SAMPLE; ++sample_idx) {
        for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
            child_sample[child_idx] = in_sample[child_idx][sample_idx];
        }
        out_sample[sample_idx] = geopm::Agg::average(child_sample);
    }
    return true;
}

// Print idle percentage to either standard out or standard error
bool ExampleAgent::adjust_platform(const std::vector<double> &in_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    // Check for NAN to set default values for policy
    double low_thresh = in_policy[M_POLICY_LOW_THRESH];
    double high_thresh = in_policy[M_POLICY_HIGH_THRESH];
    if (std::isnan(low_thresh)) {
        low_thresh = 0.30;
    }
    if (std::isnan(high_thresh)) {
        high_thresh = 0.70;
    }

    double idle_percent = m_last_sample[M_SAMPLE_IDLE_PCT];
    if (std::isnan(idle_percent)) {
        return false;
    }
    if (idle_percent < low_thresh) {
        m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL_STDERR], idle_percent);
    }
    else if (idle_percent > high_thresh) {
        m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL_STDERR], idle_percent);
    }
    else {
        m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL_STDOUT], idle_percent);
    }
    return true;
}

// Read signals from the platform and calculate samples to be sent up
bool ExampleAgent::sample_platform(std::vector<double> &out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);
    // Collect latest times from platform signals
    double total = 0.0;
    for (auto signal_idx : m_signal_idx) {
        m_last_signal[signal_idx] = m_platform_io.sample(signal_idx);
        total += m_last_signal[signal_idx];
    }

    // Update samples
    m_last_sample[M_SAMPLE_USER_PCT] = m_last_signal[M_PLAT_SIGNAL_USER] / total;
    m_last_sample[M_SAMPLE_SYSTEM_PCT] = m_last_signal[M_PLAT_SIGNAL_SYSTEM] / total;
    m_last_sample[M_SAMPLE_IDLE_PCT] = m_last_signal[M_PLAT_SIGNAL_IDLE] / total;
    out_sample[M_SAMPLE_USER_PCT] = m_last_sample[M_SAMPLE_USER_PCT];
    out_sample[M_SAMPLE_SYSTEM_PCT] = m_last_sample[M_SAMPLE_SYSTEM_PCT];
    out_sample[M_SAMPLE_IDLE_PCT] = m_last_sample[M_SAMPLE_IDLE_PCT];

    // Update mix and max for the report
    if (std::isnan(m_min_idle) || m_last_sample[M_SAMPLE_IDLE_PCT] < m_min_idle) {
        m_min_idle = m_last_sample[M_SAMPLE_IDLE_PCT];
    }
    if (std::isnan(m_max_idle) || m_last_sample[M_SAMPLE_IDLE_PCT] > m_max_idle) {
        m_max_idle = m_last_sample[M_SAMPLE_IDLE_PCT];
    }
    return true;
}

// Wait for the remaining cycle time to keep Controller loop cadence at 1 second
void ExampleAgent::wait(void)
{
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
    }
    while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
    geopm_time(&m_last_wait);
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > ExampleAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

// Adds min and max idle percentage to the per-node section of the report
std::vector<std::pair<std::string, std::string> > ExampleAgent::report_node(void) const
{
    return {
        {"Lowest idle %", std::to_string(m_min_idle)},
        {"Highest idle %", std::to_string(m_max_idle)}
    };
}

// This Agent does not add any per-region details
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > ExampleAgent::report_region(void) const
{
    return {};
}

// Adds trace columns samples and signals of interest
std::vector<std::string> ExampleAgent::trace_names(void) const
{
    return {"user_percent", "system_percent", "idle_percent",
            "user", "system", "idle", "nice"};
}

// Updates the trace with values for samples and signals from this Agent
void ExampleAgent::trace_values(std::vector<double> &values)
{
    // Sample values generated at last call to sample_platform
    values[M_TRACE_VAL_USER_PCT] = m_last_sample[M_SAMPLE_USER_PCT];
    values[M_TRACE_VAL_SYSTEM_PCT] = m_last_sample[M_SAMPLE_SYSTEM_PCT];
    values[M_TRACE_VAL_IDLE_PCT] = m_last_sample[M_SAMPLE_IDLE_PCT];
    // Signals measured at last call to sample_platform()
    values[M_TRACE_VAL_SIGNAL_USER] = m_last_signal[M_PLAT_SIGNAL_USER];
    values[M_TRACE_VAL_SIGNAL_SYSTEM] = m_last_signal[M_PLAT_SIGNAL_SYSTEM];
    values[M_TRACE_VAL_SIGNAL_IDLE] = m_last_signal[M_PLAT_SIGNAL_IDLE];
    values[M_TRACE_VAL_SIGNAL_NICE] = m_last_signal[M_PLAT_SIGNAL_NICE];
}

// Name used for registration with the Agent factory
std::string ExampleAgent::plugin_name(void)
{
    return "example";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> ExampleAgent::make_plugin(void)
{
    return geopm::make_unique<ExampleAgent>();
}

// Describes expected policies to be provided by the resource manager or user
std::vector<std::string> ExampleAgent::policy_names(void)
{
    return {"LOW_THRESHOLD", "HIGH_THRESHOLD"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> ExampleAgent::sample_names(void)
{
    return {"USER_PERCENT", "SYSTEM_PERCENT", "IDLE_PERCENT"};
}
