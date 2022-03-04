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
    , M_WAIT_SEC(0.020) // 20ms Wait
    , M_POLICY_PHI_DEFAULT(0.5)
    , m_do_write_batch(false)
    // This agent approach is meant to allow for quick prototyping through simplifying
    // signal & control addition and usage.  Most changes to signals and controls
    // should be accomplishable with changes to the declaration below (instead of updating
    // init_platform_io, sample_platform, etc).  Signal & control usage is still
    // handled in adjust_platform per usual.
    , m_signal_available({
                          {"GPU_FREQUENCY_STATUS", {         // Name of signal to be queried
                              GEOPM_DOMAIN_BOARD_ACCELERATOR, // Domain for the signal
                              true,                           // Should the signal appear in the trace
                              {}                              // Empty Vector to contain the signal info
                              }},
                          {"GPU_COMPUTE_ACTIVITY", {
                              GEOPM_DOMAIN_BOARD_ACCELERATOR,
                              true,
                              {}
                              }},
                          {"GPU_MEMORY_ACTIVITY", {
                              GEOPM_DOMAIN_BOARD_ACCELERATOR,
                              true,
                              {}
                              }},
                          {"GPU_UTILIZATION", {
                              GEOPM_DOMAIN_BOARD_ACCELERATOR,
                              true,
                              {}
                              }},
                          {"GPU_ENERGY", {
                              GEOPM_DOMAIN_BOARD_ACCELERATOR,
                              true,
                              {}
                              }},
                          {"GPU_POWER", {
                              GEOPM_DOMAIN_BOARD_ACCELERATOR,
                              true,
                              {}
                              }},
                          ///CPU SIGNALS BELOW
                          {"POWER_PACKAGE", {
                              GEOPM_DOMAIN_BOARD,
                              true,
                              {}
                              }},
                          {"POWER_DRAM", {
                              GEOPM_DOMAIN_BOARD_MEMORY,
                              true,
                              {}
                              }},
                          {"FREQUENCY", { //TODO: should move to CPU_FREQUENCY_STATUS
                              GEOPM_DOMAIN_BOARD,
                              true,
                              {}
                              }},
                          {"TEMPERATURE_PACKAGE", {
                              GEOPM_DOMAIN_BOARD,
                              true,
                              {}
                              }},
                          {"ENERGY_DRAM", {
                              GEOPM_DOMAIN_BOARD_MEMORY,
                              true,
                              {}
                              }},
                          {"INSTRUCTIONS_RETIRED", {
                              GEOPM_DOMAIN_BOARD,
                              true,
                              {}
                              }},
                          {"INSTRUCTIONS_RETIRED", {
                              GEOPM_DOMAIN_PACKAGE,
                              true,
                              {}
                              }},
                          {"CYCLES_REFERENCE", {
                              GEOPM_DOMAIN_BOARD,
                              true,
                              {}
                              }},
                          {"MSR::UNCORE_PERF_STATUS:FREQ", {
                              GEOPM_DOMAIN_PACKAGE,
                              true,
                              {}
                              }},
                          {"QM_CTR_SCALED_RATE", {
                              GEOPM_DOMAIN_PACKAGE,
                              true,
                              {}
                              }},
                          {"ENERGY_PACKAGE", {
                              GEOPM_DOMAIN_PACKAGE,
                              true,
                              {}
                              }},
                          {"MSR::APERF:ACNT", {
                              GEOPM_DOMAIN_PACKAGE,
                              true,
                              {}
                              }},
                          {"MSR::MPERF:MCNT", {
                              GEOPM_DOMAIN_PACKAGE,
                              true,
                              {}
                              }},
                          {"MSR::PPERF:PCNT", {
                              GEOPM_DOMAIN_PACKAGE,
                              false,
                              {}
                              }},
                         })
    , m_control_available({
                           {"GPU_FREQUENCY_CONTROL", {
                                GEOPM_DOMAIN_BOARD_ACCELERATOR,
                                false,
                                {}
                                }},
                            //TODO: Add CPU controls
                         })
    , m_cpu_nn_exists(true)
    , m_cpu_nn_path("cpu_control.kt")
    , m_gpu_nn_exists(true)
    , m_gpu_nn_path("gpu_control.kt")
    , m_gpu_coarse_metrics(true)
    , m_gpu_fine_metrics(true)
    , m_gpu_controls(true)
{
    geopm_time(&m_last_wait);
}

// Push signals and controls for future batch read/write
void TorchAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    m_accelerator_frequency_requests = 0;

    //TODO: check that GPU NN path exits, then try to load
    try {
        //m_gpu_neural_net = torch::jit::load(m_gpu_nn_path);

        //Using a NN per GPU.
        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR); ++domain_idx) {
            m_gpu_neural_net.push_back(torch::jit::load(m_gpu_nn_path));
        }
    }
    catch (const c10::Error& e) {
        m_gpu_nn_exists = false;
        std::cerr << "Failed to load GPU NN" << std::endl;
    }

    //TODO: check that CPU NN path exits, then try to load

    try {
        m_cpu_neural_net = torch::jit::load(m_cpu_nn_path);
    }
    catch (const c10::Error& e) {
        m_cpu_nn_exists = false;
        std::cerr << "Failed to load CPU NN" << std::endl;
    }

    if (!m_cpu_nn_exists && !m_gpu_nn_exists) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                               "(): Failed to load GPU Neural Net: " +
                               m_gpu_nn_path + " and CPU Neural Net: " +
                               m_cpu_nn_path + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    init_platform_io();
}

void TorchAgent::init_platform_io(void)
{
    // populate signals for each domain with batch idx info, default values, etc
    for (auto &sv : m_signal_available) {
        //confirm signal exists, push back for future usage if it does
        auto all_names = m_platform_io.signal_names();
        if (all_names.find(sv.first) != all_names.end()) {
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(sv.second.domain); ++domain_idx) {
                signal sgnl = signal{m_platform_io.push_signal(sv.first,
                                                               sv.second.domain,
                                                               domain_idx), NAN, NAN};
                sv.second.signals.push_back(sgnl);
            }
        }
        else {
            std::cerr << "Skipping signal: " << sv.first << std::endl;
            if(sv.first == "GPU_POWER") {
                m_gpu_coarse_metrics = false;
            }
            else if(sv.first == "GPU_COMPUTE_ACTIVITY") {
                m_gpu_fine_metrics = false;
            }
        }
    }

    // populate controls for each domain
    for (auto &sv : m_control_available) {
        //confirm signal exists, push back for future usage if it does
        auto all_names = m_platform_io.control_names();
        if (all_names.find(sv.first) != all_names.end()) {
            for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(sv.second.domain); ++domain_idx) {
                control ctrl = control{m_platform_io.push_control(sv.first,
                                                                  sv.second.domain,
                                                                  domain_idx), NAN};
                sv.second.controls.push_back(ctrl);
            }
        }
        else {
            std::cerr << "Skipping control: " << sv.first << std::endl;
            if(sv.first == "GPU_FREQUENCY_CONTROL") {
                m_gpu_controls = false;
            }
        }
    }

    auto all_names = m_platform_io.control_names();
    if (all_names.find("DCGM::FIELD_UPDATE_RATE") != all_names.end()) {
        // DCGM documentation indicates that users should query no faster than 100ms
        // even though the interface allows for setting the polling rate in the us range.
        // In practice reducing below the 100ms value has proven functional, but should only
        // be attempted if there is a proven need to catch short phase behavior that cannot
        // be accomplished with the default settings.
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

    ///////////////////////
    //CPU POLICY CHECKING//
    ///////////////////////
    // If no phi value is provided assume the default behavior.
    if (std::isnan(in_policy[M_POLICY_CPU_PHI])) {
        in_policy[M_POLICY_CPU_PHI] = M_POLICY_PHI_DEFAULT;
    }

    if (in_policy[M_POLICY_CPU_PHI] < 0.0 ||
        in_policy[M_POLICY_CPU_PHI] > 1.0) {
        throw geopm::Exception("TorchAgent::" + std::string(__func__) +
                               "(): POLICY_CPU_PHI value out of range: " +
                               std::to_string(in_policy[M_POLICY_CPU_PHI]) + ".",
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

    if (m_gpu_nn_exists) {
        // Primary signal used for frequency recommendation
        auto gpu_frequency_itr = m_signal_available.find("GPU_FREQUENCY_STATUS");
        auto gpu_power_itr = m_signal_available.find("GPU_POWER");
        auto gpu_utilization_itr = m_signal_available.find("GPU_UTILIZATION");
        auto gpu_compute_active_itr = m_signal_available.find("GPU_COMPUTE_ACTIVITY");
        auto gpu_mem_active_itr = m_signal_available.find("GPU_MEMORY_ACTIVITY");

        double gpu_freq = 0;
        double gpu_power = 0;
        double gpu_util = 0;
        double gpu_compute_active = 0;
        double gpu_mem_active = 0;

        for (int domain_idx = 0; domain_idx < m_platform_topo.num_domain(GEOPM_DOMAIN_BOARD_ACCELERATOR); ++domain_idx) {
            if (m_gpu_coarse_metrics) {
                gpu_freq = gpu_frequency_itr->second.signals.at(domain_idx).m_last_sample;
                //std::cout << "freq: " << std::to_string(gpu_freq) << std::endl;
                gpu_power = gpu_power_itr->second.signals.at(domain_idx).m_last_sample;
                //std::cout << "power: " << std::to_string(gpu_power) << std::endl;
                gpu_util = gpu_utilization_itr->second.signals.at(domain_idx).m_last_sample;
                //std::cout << "util: " << std::to_string(gpu_util) << std::endl;
            }
            if (m_gpu_fine_metrics) {
                gpu_compute_active = gpu_compute_active_itr->second.signals.at(domain_idx).m_last_sample;
                //std::cout << "compute: " << std::to_string(gpu_compute_active) << std::endl;
                gpu_mem_active = gpu_mem_active_itr->second.signals.at(domain_idx).m_last_sample;
                //std::cout << "mem: " << std::to_string(gpu_mem_active) << std::endl;
            }

            torch::Tensor xs = torch::zeros({6});
            xs[0] = gpu_freq;
            xs[1] = gpu_power;
            xs[2] = gpu_util;
            xs[3] = gpu_compute_active;
            xs[4] = gpu_mem_active;
            xs[5] = in_policy[M_POLICY_GPU_PHI];

            std::vector<torch::jit::IValue>temp_op;
            temp_op.push_back(xs);

            at::Tensor output = m_gpu_neural_net.at(domain_idx).forward(temp_op).toTensor();
            gpu_freq_request.push_back(output[0].item<double>() * 1e9); // Just assuming we need to convert
            //std::cout << "GPU" << std::to_string(domain_idx) << " recommended freq is " << std::to_string(gpu_freq_request.at(domain_idx)) << std::endl;
        }

        if (!gpu_freq_request.empty() && m_gpu_controls) {
            // set frequency control per accelerator
            auto freq_ctl_itr = m_control_available.find("GPU_FREQUENCY_CONTROL");
            for (int domain_idx = 0; domain_idx < (int) freq_ctl_itr->second.controls.size(); ++domain_idx) {
                if (gpu_freq_request.at(domain_idx) !=
                    freq_ctl_itr->second.controls.at(domain_idx).m_last_setting &&
                    !std::isnan(gpu_freq_request.at(domain_idx))) {
                    //Adjust
                    m_platform_io.adjust(freq_ctl_itr->second.controls.at(domain_idx).m_batch_idx,
                                         gpu_freq_request.at(domain_idx));

                    //save the value for future comparison
                    freq_ctl_itr->second.controls.at(domain_idx).m_last_setting =
                                         gpu_freq_request.at(domain_idx);
                    ++m_accelerator_frequency_requests;
                }
            }
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
    for (auto &sv : m_signal_available) {
        for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
            double curr_value = m_platform_io.sample(sv.second.signals.at(domain_idx).m_batch_idx);

            if (sv.first == "GPU_ENERGY" ||
                sv.first == "ENERGY_PACKAGE" ||
                sv.first == "ENERGY_DRAM") {
                sv.second.signals.at(domain_idx).m_last_sample = curr_value -
                                                                 sv.second.signals.at(domain_idx).m_last_signal;
            }
            else {
                sv.second.signals.at(domain_idx).m_last_sample = sv.second.signals.at(domain_idx).m_last_signal;
            }
            sv.second.signals.at(domain_idx).m_last_signal = curr_value;
        }
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

    result.push_back({"Accelerator Frequency Requests", std::to_string(m_accelerator_frequency_requests)});
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
    std::vector<std::string> names;

    // Signals
    // Automatically build name in the format: "FREQUENCY_ACCELERATOR-board_accelerator-0"
    for (auto &sv : m_signal_available) {
        if (sv.second.trace_signal) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                names.push_back(sv.first + "-" + m_platform_topo.domain_type_to_name(sv.second.domain) + "-" + std::to_string(domain_idx));
            }
        }
    }
    // Controls
    // Automatically build name in the format: "FREQUENCY_ACCELERATOR_CONTROL-board_accelerator-0"
    for (auto &sv : m_control_available) {
        if (sv.second.trace_control) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.controls.size(); ++domain_idx) {
                names.push_back(sv.first + "-" + m_platform_topo.domain_type_to_name(sv.second.domain) + "-" + std::to_string(domain_idx));
            }
        }
    }

    return names;

}

// Updates the trace with values for signals from this Agent
void TorchAgent::trace_values(std::vector<double> &values)
{
    int values_idx = 0;

    //Signal values are added to the trace by this agent, not sample values.
    for (auto &sv : m_signal_available) {
        if (sv.second.trace_signal) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                values[values_idx] = sv.second.signals.at(domain_idx).m_last_signal;
                ++values_idx;
            }
        }
    }

    for (auto &sv : m_control_available) {
        if (sv.second.trace_control) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.controls.size(); ++domain_idx) {
                values[values_idx] = sv.second.controls.at(domain_idx).m_last_setting;
                ++values_idx;
            }
        }
    }
}

std::vector<std::function<std::string(double)> > TorchAgent::trace_formats(void) const
{
    std::vector<std::function<std::string(double)>> trace_formats;
    for (auto &sv : m_signal_available) {
        if (sv.second.trace_signal) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.signals.size(); ++domain_idx) {
                trace_formats.push_back(m_platform_io.format_function(sv.first));
            }
        }
    }

    for (auto &sv : m_control_available) {
        if (sv.second.trace_control) {
            for (int domain_idx = 0; domain_idx < (int) sv.second.controls.size(); ++domain_idx) {
                trace_formats.push_back(m_platform_io.format_function(sv.first));
            }
        }
    }

    return trace_formats;
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
    return {"GPU_FREQ_MIN", "GPU_FREQ_MAX", "GPU_PHI", "CPU_PHI"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> TorchAgent::sample_names(void)
{
    return {};
}
