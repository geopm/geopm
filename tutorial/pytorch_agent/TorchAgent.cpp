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

#include "TorchAgent.hpp"

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
    geopm::agent_factory().register_plugin(TorchAgent::plugin_name(),
                                           TorchAgent::make_plugin,
                                           Agent::make_dictionary(TorchAgent::policy_names(),
                                                                  TorchAgent::sample_names()));
}


TorchAgent::TorchAgent()
    : TorchAgent(geopm::platform_io(), geopm::platform_topo())
{

}

TorchAgent::TorchAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
    : m_platform_io(plat_io)
    , m_platform_topo(topo)
    , m_last_wait{{0, 0}}
    , M_WAIT_SEC(0.050) // 50ms Wait
    , M_POLICY_PHI_DEFAULT(0.5)
    , M_NUM_GPU(m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR))
    , m_do_write_batch(false)
    , m_gpu_nn_path("gpu_control.pt")
{
    geopm_time(&m_last_wait);
}

// Push signals and controls for future batch read/write
void TorchAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    m_gpu_frequency_requests = 0;

    try {
        for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
            m_gpu_neural_net.push_back(torch::jit::load(m_gpu_nn_path));
        }
    }
    catch (const c10::Error& e) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                               "(): Failed to load GPU Neural Net: " +
                               m_gpu_nn_path + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    init_platform_io();
}

void TorchAgent::init_platform_io(void)
{

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_freq_status.push_back({m_platform_io.push_signal("GPU_FREQUENCY_STATUS",
                                     GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                     domain_idx), NAN});
        m_gpu_compute_activity.push_back({m_platform_io.push_signal("GPU_COMPUTE_ACTIVITY",
                                          GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                          domain_idx), NAN});
        m_gpu_memory_activity.push_back({m_platform_io.push_signal("GPU_MEMORY_ACTIVITY",
                                          GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                          domain_idx), NAN});
        m_gpu_utilization.push_back({m_platform_io.push_signal("GPU_UTILIZATION",
                                     GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                     domain_idx), NAN});
        m_gpu_power.push_back({m_platform_io.push_signal("GPU_POWER",
                                GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                domain_idx), NAN});
    }

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_freq_control.push_back({m_platform_io.push_control("GPU_FREQUENCY_CONTROL",
                                     GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                     domain_idx), NAN});
    }

    auto all_names = m_platform_io.control_names();
    if (all_names.find("DCGM::FIELD_UPDATE_RATE") != all_names.end()) {
        //Configuration of DCGM to recommended values for this agent
        m_platform_io.write_control("DCGM::FIELD_UPDATE_RATE", GEOPM_DOMAIN_BOARD, 0, 0.1); //100ms
        m_platform_io.write_control("DCGM::MAX_STORAGE_TIME", GEOPM_DOMAIN_BOARD, 0, 1);
        m_platform_io.write_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, 0, 100);
    }
}

// Validate incoming policy and configure default policy requests.
void TorchAgent::validate_policy(std::vector<double> &in_policy) const
{
    assert(in_policy.size() == M_NUM_POLICY);
    double gpu_min_freq = m_platform_io.read_signal("GPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
    double gpu_max_freq = m_platform_io.read_signal("GPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

    ///////////////////////
    //GPU POLICY CHECKING//
    ///////////////////////
    // Check for NAN to set default values for policy
    if (std::isnan(in_policy[M_POLICY_GPU_FREQ_MAX])) {
        in_policy[M_POLICY_GPU_FREQ_MAX] = gpu_max_freq;
    }

    if (in_policy[M_POLICY_GPU_FREQ_MAX] > gpu_max_freq ||
        in_policy[M_POLICY_GPU_FREQ_MAX] < gpu_min_freq ) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                        "(): GPU_FREQ_MAX out of range: " +
                        std::to_string(in_policy[M_POLICY_GPU_FREQ_MAX]) +
                        ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    // Check for NAN to set default values for policy
    if (std::isnan(in_policy[M_POLICY_GPU_FREQ_MIN])) {
        in_policy[M_POLICY_GPU_FREQ_MIN] = gpu_min_freq;
    }

    if (in_policy[M_POLICY_GPU_FREQ_MIN] > gpu_max_freq ||
        in_policy[M_POLICY_GPU_FREQ_MIN] < gpu_min_freq ) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                        "(): GPU_FREQ_MIN out of range: " +
                        std::to_string(in_policy[M_POLICY_GPU_FREQ_MIN]) +
                        ".", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    if (in_policy[M_POLICY_GPU_FREQ_MIN] > in_policy[M_POLICY_GPU_FREQ_MAX]) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                        "(): GPU_FREQ_MIN (" +
                        std::to_string(in_policy[M_POLICY_GPU_FREQ_MIN]) +
                        ") value exceeds GPU_FREQ_MAX (" +
                        std::to_string(in_policy[M_POLICY_GPU_FREQ_MAX]) +
                        ").", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    // If no phi value is provided assume the default behavior.
    if (std::isnan(in_policy[M_POLICY_GPU_PHI])) {
        in_policy[M_POLICY_GPU_PHI] = M_POLICY_PHI_DEFAULT;
    }

    if (in_policy[M_POLICY_GPU_PHI] < 0.0 ||
        in_policy[M_POLICY_GPU_PHI] > 1.0) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                               "(): POLICY_GPU_PHI value out of range: " +
                               std::to_string(in_policy[M_POLICY_GPU_PHI]) + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
}

// Distribute incoming policy to children
void TorchAgent::split_policy(const std::vector<double>& in_policy,
                                std::vector<std::vector<double> >& out_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
}

// Indicate whether to send the policy down to children
bool TorchAgent::do_send_policy(void) const
{
    return true;
}

void TorchAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                    std::vector<double>& out_sample)
{

}

// Indicate whether to send samples up to the parent
bool TorchAgent::do_send_sample(void) const
{
    return false;
}

void TorchAgent::adjust_platform(const std::vector<double>& in_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);

    m_do_write_batch = false;

    // Per GPU freq
    std::vector<double> gpu_freq_request;

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        //Create an input tensor
        torch::Tensor xs = torch::tensor({{m_gpu_freq_status.at(domain_idx).value,
                                          m_gpu_power.at(domain_idx).value,
                                          m_gpu_utilization.at(domain_idx).value,
                                          m_gpu_compute_activity.at(domain_idx).value,
                                          m_gpu_memory_activity.at(domain_idx).value,
                                          in_policy[M_POLICY_GPU_PHI]}});

        //Push tensor into IValue vector
        std::vector<torch::jit::IValue> model_input;
        model_input.push_back(xs);

        //Evaluate
        at::Tensor output = m_gpu_neural_net.at(domain_idx).forward(model_input).toTensor();

        //Save recommendation and convert to SI units for later application
        gpu_freq_request.push_back(output[0].item<double>() * 1e9);
    }

    // set frequency control per accelerator
    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        //NAN --> Max Frequency
        if(std::isnan(gpu_freq_request.at(domain_idx))) {
            gpu_freq_request.at(domain_idx) = in_policy[M_POLICY_GPU_FREQ_MAX];
        }

        if (gpu_freq_request.at(domain_idx) !=
            m_gpu_freq_control.at(domain_idx).last_setting) {
            //Adjust
            m_platform_io.adjust(m_gpu_freq_control.at(domain_idx).batch_idx,
                                 gpu_freq_request.at(domain_idx));

            //save the value for future comparison
            m_gpu_freq_control.at(domain_idx).last_setting = gpu_freq_request.at(domain_idx);

            ++m_gpu_frequency_requests;
            m_do_write_batch = true;
        }
    }
}

// If controls have a valid updated value write them.
bool TorchAgent::do_write_batch(void) const
{
    return m_do_write_batch;
}

// Read signals from the platform and calculate samples to be sent up
void TorchAgent::sample_platform(std::vector<double> &out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);

    // Collect latest signal values
    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_freq_status.at(domain_idx).value = m_platform_io.sample(m_gpu_freq_status.at(
                                                                      domain_idx).batch_idx);
        m_gpu_compute_activity.at(domain_idx).value = m_platform_io.sample(m_gpu_compute_activity.at(
                                                                           domain_idx).batch_idx);
        m_gpu_memory_activity.at(domain_idx).value = m_platform_io.sample(m_gpu_memory_activity.at(
                                                                           domain_idx).batch_idx);
        m_gpu_utilization.at(domain_idx).value = m_platform_io.sample(m_gpu_utilization.at(
                                                                      domain_idx).batch_idx);
        m_gpu_power.at(domain_idx).value = m_platform_io.sample(m_gpu_power.at(
                                                                 domain_idx).batch_idx);
    }
}

// Wait for the remaining cycle time to keep Controller loop cadence
void TorchAgent::wait(void)
{
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
    }
    while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
    geopm_time(&m_last_wait);
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > TorchAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

// Adds number of frquency requests to the per-node section of the report
std::vector<std::pair<std::string, std::string> > TorchAgent::report_host(void) const
{
    std::vector<std::pair<std::string, std::string> > result;

    result.push_back({"Accelerator Frequency Requests", std::to_string(m_gpu_frequency_requests)});
    return result;
}

// This Agent does not add any per-region details
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > TorchAgent::report_region(void) const
{
    return {};
}

// Adds trace columns signals of interest
std::vector<std::string> TorchAgent::trace_names(void) const
{
    return {};
}

// Updates the trace with values for signals from this Agent
void TorchAgent::trace_values(std::vector<double> &values)
{
}

std::vector<std::function<std::string(double)> > TorchAgent::trace_formats(void) const
{
    return {};
}

// Name used for registration with the Agent factory
std::string TorchAgent::plugin_name(void)
{
    return "torch";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> TorchAgent::make_plugin(void)
{
    return geopm::make_unique<TorchAgent>();
}

// Describes expected policies to be provided by the resource manager or user
std::vector<std::string> TorchAgent::policy_names(void)
{
    return {"GPU_FREQ_MIN", "GPU_FREQ_MAX", "GPU_PHI"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> TorchAgent::sample_names(void)
{
    return {};
}
