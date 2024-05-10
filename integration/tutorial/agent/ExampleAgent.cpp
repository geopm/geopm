/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ExampleAgent.hpp"

#include <cmath>
#include <cassert>
#include <algorithm>

#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Waiter.hpp"
#include "geopm/Environment.hpp"

using geopm::Agent;
using geopm::PlatformIO;
using geopm::PlatformTopo;

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
    , M_WAIT_SEC(1.0)
    , m_waiter(geopm::Waiter::make_unique(geopm::environment().period(M_WAIT_SEC)))
    , m_min_idle(NAN)
    , m_max_idle(NAN)
    , m_is_control_active(false)
{

}

// Push signals and controls for future batch read/write
void ExampleAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    // all signals and controls will be at board domain
    int board = GEOPM_DOMAIN_BOARD;
    // push signals
    m_signal_idx[M_PLAT_SIGNAL_USER] = m_platform_io.push_signal("USER_TIME", board, 0);
    m_signal_idx[M_PLAT_SIGNAL_SYSTEM] = m_platform_io.push_signal("SYSTEM_TIME", board, 0);
    m_signal_idx[M_PLAT_SIGNAL_IDLE] = m_platform_io.push_signal("IDLE_TIME", board, 0);
    m_signal_idx[M_PLAT_SIGNAL_NICE] = m_platform_io.push_signal("NICE_TIME", board, 0);
    // push controls
    if (m_platform_io.control_names().count("TMP_FILE_CONTROL") != 0) {
        m_control_idx[M_PLAT_CONTROL] = m_platform_io.push_control("TMP_FILE_CONTROL", board, 0);
        m_is_control_active = true;
    }
    else {
        m_control_idx[M_PLAT_CONTROL] = -1;
    }

}

// Validate incoming policy and configure default policy requests.
void ExampleAgent::validate_policy(std::vector<double> &in_policy) const
{
    assert(in_policy.size() == M_NUM_POLICY);
}

// Distribute incoming policy to children
void ExampleAgent::split_policy(const std::vector<double>& in_policy,
                                std::vector<std::vector<double> >& out_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
}

// Indicate whether to send the policy down to children
bool ExampleAgent::do_send_policy(void) const
{
    return true;
}

// Aggregate average utilization samples from children
void ExampleAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                    std::vector<double>& out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);
    std::vector<double> child_sample(in_sample.size());
    for (size_t sample_idx = 0; sample_idx < M_NUM_SAMPLE; ++sample_idx) {
        for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
            child_sample[child_idx] = in_sample[child_idx][sample_idx];
        }
        out_sample[sample_idx] = geopm::Agg::average(child_sample);
    }
}

// Indicate whether to send samples up to the parent
bool ExampleAgent::do_send_sample(void) const
{
    return true;
}

// Set temporary file to 0 if in range, or percent out of range otherwise
void ExampleAgent::adjust_platform(const std::vector<double>& in_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    // Check for NAN to set default values for policy
    double low_thresh = in_policy[M_POLICY_LOW_THRESH];
    double high_thresh = in_policy[M_POLICY_HIGH_THRESH];
    if (!geopm_pio_check_valid_value(low_thresh)) {
        low_thresh = 0.30;
    }
    if (!geopm_pio_check_valid_value(high_thresh)) {
        high_thresh = 0.70;
    }

    double idle_percent = m_last_sample[M_SAMPLE_IDLE_PCT];
    if (m_is_control_active && geopm_pio_check_valid_value(idle_percent)) {
        if (idle_percent < low_thresh) {
            m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL], idle_percent - low_thresh);
        }
        else if (idle_percent > high_thresh) {
            m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL], idle_percent - high_thresh);
        }
        else {
            m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL], 0);
        }
    }
}

// If idle percent had a valid value, execute the print
bool ExampleAgent::do_write_batch(void) const
{
    return m_is_control_active && geopm_pio_check_valid_value(m_last_sample[M_SAMPLE_IDLE_PCT]);
}

// Read signals from the platform and calculate samples to be sent up
void ExampleAgent::sample_platform(std::vector<double> &out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);
    // Collect latest times from platform signals
    double total = 0.0;
    auto signal_it = m_last_signal.begin();
    for (auto signal_idx : m_signal_idx) {
        *signal_it = m_platform_io.sample(signal_idx);
        total += *signal_it;
        ++signal_it;
    }

    // Update samples
    double factor = 0.0;
    if (total != 0) {
        factor = 1.0 / total;
    }
    m_last_sample[M_SAMPLE_USER_PCT] = m_last_signal[M_PLAT_SIGNAL_USER] * factor;
    m_last_sample[M_SAMPLE_SYSTEM_PCT] = m_last_signal[M_PLAT_SIGNAL_SYSTEM] * factor;
    m_last_sample[M_SAMPLE_IDLE_PCT] = m_last_signal[M_PLAT_SIGNAL_IDLE] * factor;
    out_sample[M_SAMPLE_USER_PCT] = m_last_sample[M_SAMPLE_USER_PCT];
    out_sample[M_SAMPLE_SYSTEM_PCT] = m_last_sample[M_SAMPLE_SYSTEM_PCT];
    out_sample[M_SAMPLE_IDLE_PCT] = m_last_sample[M_SAMPLE_IDLE_PCT];

    // Update min and max for the report
    if (!geopm_pio_check_valid_value(m_min_idle) || m_last_sample[M_SAMPLE_IDLE_PCT] < m_min_idle) {
        m_min_idle = m_last_sample[M_SAMPLE_IDLE_PCT];
    }
    if (!geopm_pio_check_valid_value(m_max_idle) || m_last_sample[M_SAMPLE_IDLE_PCT] > m_max_idle) {
        m_max_idle = m_last_sample[M_SAMPLE_IDLE_PCT];
    }
}

// Wait for the remaining cycle time to keep Controller loop cadence at 1 second
void ExampleAgent::wait(void)
{
    m_waiter->wait();
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > ExampleAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

// Adds min and max idle percentage to the per-node section of the report
std::vector<std::pair<std::string, std::string> > ExampleAgent::report_host(void) const
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

std::vector<std::function<std::string(double)> > ExampleAgent::trace_formats(void) const
{
    return {geopm::string_format_float,    // M_TRACE_VAL_USER_PCT
            geopm::string_format_float,    // M_TRACE_VAL_SYSTEM_PCT
            geopm::string_format_float,    // M_TRACE_VAL_IDLE_PCT
            geopm::string_format_double,   // M_TRACE_VAL_SIGNAL_USER
            geopm::string_format_double,   // M_TRACE_VAL_SIGNAL_SYSTEM
            geopm::string_format_double,   // M_TRACE_VAL_SIGNAL_IDLE
            geopm::string_format_double,   // M_TRACE_VAL_SIGNAL_NICE
            };
}

void ExampleAgent::enforce_policy(const std::vector<double> &policy) const
{

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
