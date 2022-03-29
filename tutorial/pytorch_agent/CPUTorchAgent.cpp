/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "CPUTorchAgent.hpp"

#include <cmath>
#include <cassert>
#include <algorithm>

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
    geopm::agent_factory().register_plugin(CPUTorchAgent::plugin_name(),
                                           CPUTorchAgent::make_plugin,
                                           Agent::make_dictionary(CPUTorchAgent::policy_names(),
                                                                  CPUTorchAgent::sample_names()));
}


CPUTorchAgent::CPUTorchAgent()
    : CPUTorchAgent(geopm::platform_io(), geopm::platform_topo())
{

}

CPUTorchAgent::CPUTorchAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
    : m_platform_io(plat_io)
    , m_platform_topo(topo)
    , m_last_wait{{0, 0}}
    , M_WAIT_SEC(0.050) // 50ms Wait
    , M_POLICY_PHI_DEFAULT(0.5)
    , M_NUM_PACKAGE(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
    , m_do_write_batch(false)
    , m_package_nn_path("cpu_control.pt")
{
    geopm_time(&m_last_wait);
}

// Push signals and controls for future batch read/write
void CPUTorchAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    m_package_frequency_requests = 0;

    try {
        for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
            m_package_neural_net.push_back(torch::jit::load(m_package_nn_path));
        }
    }
    catch (const c10::Error& e) {
        throw geopm::Exception("CPUTorchAgent::" + std::string(__func__) +
                               "(): Failed to load Neural Net: " +
                               m_package_nn_path + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    init_platform_io();
}

void CPUTorchAgent::init_platform_io(void)
{
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_package_power.push_back({m_platform_io.push_signal("POWER_PACKAGE",
                                   GEOPM_DOMAIN_PACKAGE,
                                   domain_idx), NAN});
        m_package_power_dram.push_back({m_platform_io.push_signal("POWER_DRAM",
                                        GEOPM_DOMAIN_PACKAGE,
                                        domain_idx), NAN});
        m_package_freq_status.push_back({m_platform_io.push_signal("CPU_FREQUENCY_STATUS",
                                         GEOPM_DOMAIN_PACKAGE,
                                         domain_idx), NAN});
        m_package_temperature.push_back({m_platform_io.push_signal("TEMPERATURE_CORE",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), NAN});
        m_package_uncore_freq_status.push_back({m_platform_io.push_signal("MSR::UNCORE_PERF_STATU:FREQ",
                                                GEOPM_DOMAIN_PACKAGE,
                                                domain_idx), NAN});
        m_package_qm_rate.push_back({m_platform_io.push_signal("QM_CTR_SCALED_RATE",
                                     GEOPM_DOMAIN_PACKAGE,
                                     domain_idx), NAN});
        m_package_inst_retired.push_back({m_platform_io.push_signal("INSTRUCTIONS_RETIRED",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), NAN});
        m_package_cycles_unhalted.push_back({m_platform_io.push_signal("CYCLES_THREAD",
                                             GEOPM_DOMAIN_PACKAGE,
                                             domain_idx), NAN});
        m_package_energy.push_back({m_platform_io.push_signal("ENERGY_PACKAGE",
                                    GEOPM_DOMAIN_PACKAGE,
                                    domain_idx), NAN});
        m_package_acnt.push_back({m_platform_io.push_signal("MSR::APERF:ACNT",
                                  GEOPM_DOMAIN_PACKAGE,
                                  domain_idx), NAN});
        m_package_mcnt.push_back({m_platform_io.push_signal("MSR::MPERF:MCNT",
                                  GEOPM_DOMAIN_PACKAGE,
                                  domain_idx), NAN});
        m_package_pcnt.push_back({m_platform_io.push_signal("MSR::PPERF:PCNT",
                                  GEOPM_DOMAIN_PACKAGE,
                                  domain_idx), NAN});
    }

    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_package_freq_control.push_back({m_platform_io.push_control("CPU_FREQUENCY_CONTROL",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), NAN});
    }

    //Configuration of QM_CTR must match QM_CTR config used for training data
    m_platform_io.write_control("MSR::PQR_ASSOC:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
    m_platform_io.write_control("MSR::QM_EVTSEL:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
    m_platform_io.write_control("MSR::QM_EVTSEL:EVENT_ID", GEOPM_DOMAIN_BOARD, 0, 2);
}

// Validate incoming policy and configure default policy requests.
void CPUTorchAgent::validate_policy(std::vector<double> &in_policy) const
{
    assert(in_policy.size() == M_NUM_POLICY);
    double min_freq = m_platform_io.read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
    double max_freq = m_platform_io.read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

    ///////////////////////
    //CPU POLICY CHECKING//
    ///////////////////////
    // Check for NAN to set default values for policy
    if (std::isnan(in_policy[M_POLICY_CPU_FREQ_MAX])) {
        in_policy[M_POLICY_CPU_FREQ_MAX] = max_freq;
    }

    if (in_policy[M_POLICY_CPU_FREQ_MAX] > max_freq ||
        in_policy[M_POLICY_CPU_FREQ_MAX] < min_freq ) {
        throw geopm::Exception("CPUTorchAgent::" + std::string(__func__) +
                        "():_FREQ_MAX out of range: " +
                        std::to_string(in_policy[M_POLICY_CPU_FREQ_MAX]) +
                        ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    // Check for NAN to set default values for policy
    if (std::isnan(in_policy[M_POLICY_CPU_FREQ_MIN])) {
        in_policy[M_POLICY_CPU_FREQ_MIN] = min_freq;
    }

    if (in_policy[M_POLICY_CPU_FREQ_MIN] > max_freq ||
        in_policy[M_POLICY_CPU_FREQ_MIN] < min_freq ) {
        throw geopm::Exception("CPUTorchAgent::" + std::string(__func__) +
                        "():_FREQ_MIN out of range: " +
                        std::to_string(in_policy[M_POLICY_CPU_FREQ_MIN]) +
                        ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    if (in_policy[M_POLICY_CPU_FREQ_MIN] > in_policy[M_POLICY_CPU_FREQ_MAX]) {
        throw geopm::Exception("CPUTorchAgent::" + std::string(__func__) +
                        "():_FREQ_MIN (" +
                        std::to_string(in_policy[M_POLICY_CPU_FREQ_MIN]) +
                        ") value exceeds_FREQ_MAX (" +
                        std::to_string(in_policy[M_POLICY_CPU_FREQ_MAX]) +
                        ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    // If no phi value is provided assume the default behavior.
    if (std::isnan(in_policy[M_POLICY_CPU_PHI])) {
        in_policy[M_POLICY_CPU_PHI] = M_POLICY_PHI_DEFAULT;
    }

    if (in_policy[M_POLICY_CPU_PHI] < 0.0 ||
        in_policy[M_POLICY_CPU_PHI] > 1.0) {
        throw geopm::Exception("CPUTorchAgent::" + std::string(__func__) +
                               "(): POLICY_CPU_PHI value out of range: " +
                               std::to_string(in_policy[M_POLICY_CPU_PHI]) + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
}

// Distribute incoming policy to children
void CPUTorchAgent::split_policy(const std::vector<double>& in_policy,
                                std::vector<std::vector<double> >& out_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
}

// Indicate whether to send the policy down to children
bool CPUTorchAgent::do_send_policy(void) const
{
    return true;
}

void CPUTorchAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                    std::vector<double>& out_sample)
{

}

// Indicate whether to send samples up to the parent
bool CPUTorchAgent::do_send_sample(void) const
{
    return false;
}

void CPUTorchAgent::adjust_platform(const std::vector<double>& in_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);

    m_do_write_batch = false;

    // Per freq
    std::vector<double> package_freq_request;

    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        //Create an input tensor
        torch::Tensor xs = torch::tensor({{m_package_power.at(domain_idx).value,
                                           m_package_power_dram.at(domain_idx).value,
                                           m_package_freq_status.at(domain_idx).value,
                                           m_package_temperature.at(domain_idx).value,
                                           m_package_uncore_freq_status.at(domain_idx).value,
                                           m_package_qm_rate.at(domain_idx).value,
                                           m_package_inst_retired.at(domain_idx).value / m_package_cycles_unhalted.at(domain_idx).value,
                                           m_package_inst_retired.at(domain_idx).value / m_package_energy.at(domain_idx).value,
                                           m_package_acnt.at(domain_idx).value / m_package_mcnt.at(domain_idx).value,
                                           m_package_pcnt.at(domain_idx).value / m_package_mcnt.at(domain_idx).value,
                                           m_package_pcnt.at(domain_idx).value / m_package_acnt.at(domain_idx).value,
                                           in_policy[M_POLICY_CPU_PHI]}});

        //Push tensor into IValue vector
        std::vector<torch::jit::IValue> model_input;
        model_input.push_back(xs);

        //Evaluate
        at::Tensor output = m_package_neural_net.at(domain_idx).forward(model_input).toTensor();

        //Save recommendation and convert to SI units for later application
        package_freq_request.push_back(output[0].item<double>() * 1e9);
    }

    // set frequency control per accelerator
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        //NAN --> Max Frequency
        if(std::isnan(package_freq_request.at(domain_idx))) {
            package_freq_request.at(domain_idx) = in_policy[M_POLICY_CPU_FREQ_MAX];
        }

        if (package_freq_request.at(domain_idx) !=
            m_package_freq_control.at(domain_idx).last_setting) {
            //Adjust
            m_platform_io.adjust(m_package_freq_control.at(domain_idx).batch_idx,
                                package_freq_request.at(domain_idx));

            //save the value for future comparison
            m_package_freq_control.at(domain_idx).last_setting = package_freq_request.at(domain_idx);

            ++m_package_frequency_requests;
            m_do_write_batch = true;
        }
    }
}

// If controls have a valid updated value write them.
bool CPUTorchAgent::do_write_batch(void) const
{
    return m_do_write_batch;
}

// Read signals from the platform and calculate samples to be sent up
void CPUTorchAgent::sample_platform(std::vector<double> &out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);

    // Collect latest signal values
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_package_power.at(domain_idx).value = m_platform_io.sample(m_package_power.at(domain_idx).batch_idx);
        m_package_power_dram.at(domain_idx).value = m_platform_io.sample(m_package_power_dram.at(domain_idx).batch_idx);
        m_package_freq_status.at(domain_idx).value = m_platform_io.sample(m_package_freq_status.at(domain_idx).batch_idx);
        m_package_temperature.at(domain_idx).value = m_platform_io.sample(m_package_temperature.at(domain_idx).batch_idx);
        m_package_uncore_freq_status.at(domain_idx).value = m_platform_io.sample(m_package_uncore_freq_status.at(domain_idx).batch_idx);
        m_package_qm_rate.at(domain_idx).value = m_platform_io.sample(m_package_qm_rate.at(domain_idx).batch_idx);
        m_package_cycles_unhalted.at(domain_idx).value = m_platform_io.sample(m_package_cycles_unhalted.at(domain_idx).batch_idx);
        m_package_inst_retired.at(domain_idx).value = m_platform_io.sample(m_package_inst_retired.at(domain_idx).batch_idx);
        m_package_acnt.at(domain_idx).value = m_platform_io.sample(m_package_acnt.at(domain_idx).batch_idx);
        m_package_mcnt.at(domain_idx).value = m_platform_io.sample(m_package_mcnt.at(domain_idx).batch_idx);
        m_package_pcnt.at(domain_idx).value = m_platform_io.sample(m_package_pcnt.at(domain_idx).batch_idx);

        //running counter
        m_package_energy.at(domain_idx).value = m_platform_io.sample(m_package_energy.at(domain_idx).batch_idx);

        //diffed energy
        //m_package_energy.at(domain_idx).value = m_platform_io.sample(m_package_energy.at(domain_idx).batch_idx) - m_package_energy.at(domain_idx).value;
    }
}

// Wait for the remaining cycle time to keep Controller loop cadence
void CPUTorchAgent::wait(void)
{
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
    }
    while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
    geopm_time(&m_last_wait);
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > CPUTorchAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

// Adds number of frquency requests to the per-node section of the report
std::vector<std::pair<std::string, std::string> > CPUTorchAgent::report_host(void) const
{
    std::vector<std::pair<std::string, std::string> > result;

    result.push_back({"Xeon Package Frequency Requests", std::to_string(m_package_frequency_requests)});
    return result;
}

// This Agent does not add any per-region details
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > CPUTorchAgent::report_region(void) const
{
    return {};
}

// Adds trace columns signals of interest
std::vector<std::string> CPUTorchAgent::trace_names(void) const
{
    return {};
}

// Updates the trace with values for signals from this Agent
void CPUTorchAgent::trace_values(std::vector<double> &values)
{
}

std::vector<std::function<std::string(double)> > CPUTorchAgent::trace_formats(void) const
{
    return {};
}

// Name used for registration with the Agent factory
std::string CPUTorchAgent::plugin_name(void)
{
    return "cpu_torch";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> CPUTorchAgent::make_plugin(void)
{
    return geopm::make_unique<CPUTorchAgent>();
}

// Describes expected policies to be provided by the resource manager or user
std::vector<std::string> CPUTorchAgent::policy_names(void)
{
    return {"CPU_FREQ_MIN", "CPU_FREQ_MAX", "CPU_PHI"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> CPUTorchAgent::sample_names(void)
{
    return {};
}
