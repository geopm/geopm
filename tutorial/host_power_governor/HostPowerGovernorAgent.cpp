/*
 * Copyright (c) 2020, Intel Corporation
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

#include <unistd.h>
#include <climits>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>
#include "geopm/Agent.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PowerGovernor.hpp"
#include "geopm_time.h"
#include <system_error>

using geopm::Agent;
using geopm::PlatformIO;
using geopm::PlatformTopo;
using geopm::PowerGovernor;

using HostName = std::string;
using PowerLimit = double;

// Get the hostname of the host running this instance of the agent.
static HostName get_host_name()
{
    char my_host_name[HOST_NAME_MAX + 1] = {};
    if (gethostname(my_host_name, HOST_NAME_MAX) != 0) {
        throw std::system_error(errno, std::generic_category(),
                                "HostPowerGovernorAgent: gethostname");
    }
    return my_host_name;
}

// Get the requested power limit of the given host.
static PowerLimit get_host_limit(const HostName &host_name)
{
    const char *env_str = std::getenv("GEOPM_HOST_POWER_LIMITS");
    if (env_str) {
        std::istringstream env_iss(env_str);
        for (std::string host_power_str; std::getline(env_iss, host_power_str, ',');) {
            std::istringstream host_power_iss(host_power_str);
            HostName host;
            std::getline(host_power_iss, host, '=');
            if (host == host_name) {
                PowerLimit power;
                host_power_iss >> power;
                return power;
            }
        }
    }
    return NAN;
}

// The HostPowerGovernorAgent is a GEOPM agent that performs per-host power
// governing. The GEOPM_HOST_POWER_LIMITS environment variable contains a
// comma-separated list of '='-separated (hostname, power limit) pairs. If no
// limit is specified for a host, the host's TDP is used instead. Requested
// limits are clamped between POWER_PACKAGE_MIN and POWER_PACKAGE_MAX.
class HostPowerGovernorAgent : public Agent
{
    public:
        HostPowerGovernorAgent();
        HostPowerGovernorAgent(PlatformIO &platform_io, const PlatformTopo &platform_topo,
                               std::unique_ptr<PowerGovernor> power_gov);
        virtual ~HostPowerGovernorAgent();
        void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
        void validate_policy(std::vector<double> &policy) const override;
        void split_policy(const std::vector<double> &in_policy,
                          std::vector<std::vector<double> > &out_policy) override;
        bool do_send_policy(void) const override;
        void aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                              std::vector<double> &out_sample) override;
        bool do_send_sample(void) const override;
        void adjust_platform(const std::vector<double> &in_policy) override;
        bool do_write_batch(void) const override;
        void sample_platform(std::vector<double> &out_sample) override;
        void wait(void) override;
        std::vector<std::pair<std::string, std::string> > report_header(void) const override;
        std::vector<std::pair<std::string, std::string> > report_host(void) const override;
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >
            report_region(void) const override;
        std::vector<std::string> trace_names(void) const override;
        std::vector<std::function<std::string(double)> > trace_formats(void) const override;
        void trace_values(std::vector<double> &values) override;
        void enforce_policy(const std::vector<double> &policy) const override;

        static std::string plugin_name(void);
        static std::unique_ptr<Agent> make_plugin(void);
        static std::vector<std::string> policy_names(void);
        static std::vector<std::string> sample_names(void);

    private:
        PlatformIO &m_platform_io;
        const PlatformTopo &m_platform_topo;
        double m_min_power_setting;
        double m_max_power_setting;
        double m_tdp_power_setting;
        std::unique_ptr<PowerGovernor> m_power_gov;
        geopm_time_s m_last_wait;
        const double M_WAIT_SEC;

        HostName m_host_name;
        PowerLimit m_host_limit;
};

// Default constructor. Creates an instance of the agent.
HostPowerGovernorAgent::HostPowerGovernorAgent()
    : HostPowerGovernorAgent(geopm::platform_io(), geopm::platform_topo(), nullptr)
{
}

// Constructor that enables injecting agent dependencies for testing.
HostPowerGovernorAgent::HostPowerGovernorAgent(PlatformIO &platform_io,
                                               const PlatformTopo &platform_topo,
                                               std::unique_ptr<PowerGovernor> power_gov)
    : m_platform_io(platform_io)
    , m_platform_topo(platform_topo)
    , m_min_power_setting(
          m_platform_io.read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_BOARD, 0))
    , m_max_power_setting(
          m_platform_io.read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_BOARD, 0))
    , m_tdp_power_setting(
          m_platform_io.read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0))
    , m_power_gov(std::move(power_gov))
    , m_last_wait(GEOPM_TIME_REF)
    , M_WAIT_SEC(0.005)
    , m_host_name(get_host_name())
    , m_host_limit(get_host_limit(m_host_name))
{
    geopm_time(&m_last_wait);

    if (std::isnan(m_host_limit)) {
        m_host_limit = m_tdp_power_setting;
    }
    if (m_host_limit < m_min_power_setting) {
        m_host_limit = m_min_power_setting;
    }
    else if (m_host_limit > m_max_power_setting) {
        m_host_limit = m_max_power_setting;
    }
}

HostPowerGovernorAgent::~HostPowerGovernorAgent() = default;

// Push signals and controls. This agent has no signals and controls of its
// own, but it delegates power governing to the PowerGovernor.
void HostPowerGovernorAgent::init(int level, const std::vector<int> &fan_in, bool is_root)
{
    if (level == 0) {
        if (!m_power_gov) {
            m_power_gov = PowerGovernor::make_unique();
        }
        m_power_gov->init_platform_io();
    }
}

// Validate the incomping policy. This agent does not use a policy.
void HostPowerGovernorAgent::validate_policy(std::vector<double> &policy) const
{
}

// Distribute an incoming policy to this agent instance's child agent
// instances. This agent does not use a policy.
void HostPowerGovernorAgent::split_policy(const std::vector<double> &in_policy,
                                          std::vector<std::vector<double> > &out_policy)
{
}

// Indicate whether to send this policy down to child agent instances. This
// agent does not use a policy.
bool HostPowerGovernorAgent::do_send_policy(void) const
{
    return false;
}

// Aggregate samples from child agent instances. This agent does not send
// samples.
void HostPowerGovernorAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                              std::vector<double> &out_sample)
{
}

// Indicate whether to send samples to the parent agent instance. This agent
// does not send samples.
bool HostPowerGovernorAgent::do_send_sample(void) const
{
    return false;
}

// Apply per-host limits
void HostPowerGovernorAgent::adjust_platform(const std::vector<double> &in_policy)
{

    PowerLimit actual_power_limit = m_host_limit;
    m_power_gov->adjust_platform(m_host_limit, actual_power_limit);

    if (m_host_limit != actual_power_limit) {
        std::ostringstream oss;
        oss << "HostPowerGovernorAgent: unable to set power limit to "
            << m_host_limit << " on " << m_host_name;
        throw std::runtime_error(oss.str());
    }
}

// Delegate do_write_batch to the power governor
bool HostPowerGovernorAgent::do_write_batch(void) const
{
    return m_power_gov->do_write_batch();
}

// Delegate sample_platform to the power governor
void HostPowerGovernorAgent::sample_platform(std::vector<double> &out_sample)
{
    m_power_gov->sample_platform();
}

// Wait for the remaining cycle time to keep Controller loop cadence at 5 ms
void HostPowerGovernorAgent::wait()
{
    while (geopm_time_since(&m_last_wait) < M_WAIT_SEC) {
    }
    geopm_time(&m_last_wait);
}

// This agent has no report header
std::vector<std::pair<std::string, std::string> >
    HostPowerGovernorAgent::report_header(void) const
{
    return {};
}

// For each host in the report, declare which power limit was applied
std::vector<std::pair<std::string, std::string> >
    HostPowerGovernorAgent::report_host(void) const
{
    return { { "HOST_POWER_LIMIT", geopm::string_format_double(m_host_limit) } };
}

// This agent has no per-region reporting.
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >
    HostPowerGovernorAgent::report_region(void) const
{
    return {};
}

// This agent does not add any signals to the trace.
std::vector<std::string> HostPowerGovernorAgent::trace_names(void) const
{
    return {};
}

// This agent does not add any signals to the trace.
std::vector<std::function<std::string(double)> >
    HostPowerGovernorAgent::trace_formats(void) const
{
    return {};
}

// This agent does not add any signals to the trace.
void HostPowerGovernorAgent::trace_values(std::vector<double> &values) {}

// Enforce the configured power limit for this host.
void HostPowerGovernorAgent::enforce_policy(const std::vector<double> &policy) const
{
    int control_domain =
        m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT");
    double pkg_policy = m_host_limit / m_platform_topo.num_domain(control_domain);
    m_platform_io.write_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_BOARD, 0, pkg_policy);
}

// Name used for registration with the Agent factory.
std::string HostPowerGovernorAgent::plugin_name(void)
{
    return "host_power_governor";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> HostPowerGovernorAgent::make_plugin(void)
{
    return geopm::make_unique<HostPowerGovernorAgent>();
}

// This agent does not use a policy, but uses the GEOPM_HOST_POWER_LIMITS
// environment variable instead.
std::vector<std::string> HostPowerGovernorAgent::policy_names(void)
{
    return {};
}

// This agent does not provide any samples.
std::vector<std::string> HostPowerGovernorAgent::sample_names(void)
{
    return {};
}

// Registers this Agent with the Agent factory, making it visible
// to the Controller when the plugin is first loaded.
static void __attribute__((constructor)) governor_agent_load(void)
{
    try {
        geopm::agent_factory().register_plugin(
            HostPowerGovernorAgent::plugin_name(), HostPowerGovernorAgent::make_plugin,
            Agent::make_dictionary(HostPowerGovernorAgent::policy_names(),
                                   HostPowerGovernorAgent::sample_names()));
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Error: unknown cause" << std::endl;
    }
}
