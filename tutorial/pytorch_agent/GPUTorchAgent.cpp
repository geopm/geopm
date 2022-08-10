/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "GPUTorchAgent.hpp"

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
    geopm::agent_factory().register_plugin(GPUTorchAgent::plugin_name(),
                                           GPUTorchAgent::make_plugin,
                                           Agent::make_dictionary(GPUTorchAgent::policy_names(),
                                                                  GPUTorchAgent::sample_names()));
}


GPUTorchAgent::GPUTorchAgent()
    : GPUTorchAgent(geopm::platform_io(), geopm::platform_topo())
{

}

GPUTorchAgent::GPUTorchAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
    : m_platform_io(plat_io)
    , m_platform_topo(topo)
    , m_last_wait{{0, 0}}
    , M_WAIT_SEC(0.050) // 50ms Wait
    , M_POLICY_PHI_DEFAULT(0.5)
    , M_GPU_ACTIVITY_CUTOFF(0.05)
    , M_NUM_GPU(m_platform_topo.num_domain(GEOPM_DOMAIN_GPU))
    , m_do_write_batch(false)
    , m_gpu_nn_path("gpu_control.pt")
{
    geopm_time(&m_last_wait);
}

// Push signals and controls for future batch read/write
void GPUTorchAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    m_gpu_frequency_requests = 0;

    char* env_nn_path = getenv("GEOPM_GPU_NN_PATH");
    if (env_nn_path != NULL) {
        std::cout << "Loading ENV: " << std::string(env_nn_path) << std::endl;
        m_gpu_nn_path = env_nn_path;
    }
    else {
        std::cerr << "GEOPM_GPU_NN_PATH env variable is NULL.  Attempting to load local gpu_control.pt" << std::endl;
    }

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_neural_net.push_back(torch::jit::load(m_gpu_nn_path));
    }

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_active_region_start.push_back(0.0);
        m_gpu_active_region_stop.push_back(0.0);
        m_gpu_active_energy_start.push_back(0.0);
        m_gpu_active_energy_stop.push_back(0.0);
    }

    init_platform_io();
}

void GPUTorchAgent::init_platform_io(void)
{

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_freq_status.push_back({m_platform_io.push_signal("GPU_CORE_FREQUENCY_STATUS",
                                     GEOPM_DOMAIN_GPU,
                                     domain_idx), NAN});
        m_gpu_compute_activity.push_back({m_platform_io.push_signal("GPU_CORE_ACTIVITY",
                                          GEOPM_DOMAIN_GPU,
                                          domain_idx), NAN});
        m_gpu_memory_activity.push_back({m_platform_io.push_signal("GPU_UNCORE_ACTIVITY",
                                          GEOPM_DOMAIN_GPU,
                                          domain_idx), NAN});
        m_gpu_utilization.push_back({m_platform_io.push_signal("GPU_UTILIZATION",
                                     GEOPM_DOMAIN_GPU,
                                     domain_idx), NAN});
        m_gpu_power.push_back({m_platform_io.push_signal("GPU_POWER",
                                GEOPM_DOMAIN_GPU,
                                domain_idx), NAN});
        m_gpu_energy.push_back({m_platform_io.push_signal("GPU_ENERGY",
                                GEOPM_DOMAIN_GPU,
                                domain_idx), NAN});
    }

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        m_gpu_freq_control.push_back({m_platform_io.push_control("GPU_CORE_FREQUENCY_CONTROL",
                                     GEOPM_DOMAIN_GPU,
                                     domain_idx), NAN});
    }

    m_time = {m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0), NAN};

    auto all_names = m_platform_io.control_names();
    if (all_names.find("DCGM::FIELD_UPDATE_RATE") != all_names.end()) {
        //Configuration of DCGM to recommended values for this agent
        m_platform_io.write_control("DCGM::FIELD_UPDATE_RATE", GEOPM_DOMAIN_BOARD, 0, 0.1); //100ms
        m_platform_io.write_control("DCGM::MAX_STORAGE_TIME", GEOPM_DOMAIN_BOARD, 0, 1);
        m_platform_io.write_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, 0, 100);
    }
}

// Validate incoming policy and configure default policy requests.
void GPUTorchAgent::validate_policy(std::vector<double> &in_policy) const
{
    assert(in_policy.size() == M_NUM_POLICY);
    double gpu_min_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
    double gpu_max_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

    ///////////////////////
    //GPU POLICY CHECKING//
    ///////////////////////
    // If no phi value is provided assume the default behavior.
    if (std::isnan(in_policy[M_POLICY_GPU_PHI])) {
        in_policy[M_POLICY_GPU_PHI] = M_POLICY_PHI_DEFAULT;
    }

    if (in_policy[M_POLICY_GPU_PHI] < 0.0 ||
        in_policy[M_POLICY_GPU_PHI] > 1.0) {
        throw geopm::Exception("GPUTorchAgent::" + std::string(__func__) +
                               "(): POLICY_GPU_PHI value out of range: " +
                               std::to_string(in_policy[M_POLICY_GPU_PHI]) + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
}

// Distribute incoming policy to children
void GPUTorchAgent::split_policy(const std::vector<double>& in_policy,
                                std::vector<std::vector<double> >& out_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
}

// Indicate whether to send the policy down to children
bool GPUTorchAgent::do_send_policy(void) const
{
    return true;
}

void GPUTorchAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                    std::vector<double>& out_sample)
{

}

// Indicate whether to send samples up to the parent
bool GPUTorchAgent::do_send_sample(void) const
{
    return false;
}

void GPUTorchAgent::adjust_platform(const std::vector<double>& in_policy)
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

        // Tracking logic.  This is not needed for any performance reason,
        // but does provide useful metrics for tracking agent behavior
        if (m_gpu_compute_activity.at(domain_idx).value >= M_GPU_ACTIVITY_CUTOFF) {
            m_gpu_active_region_stop.at(domain_idx) = 0;
            if (m_gpu_active_region_start.at(domain_idx) == 0) {
                m_gpu_active_region_start.at(domain_idx) = m_time.value;
                m_gpu_active_energy_start.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
            }
        }
        else {
            if (m_gpu_active_region_stop.at(domain_idx) == 0) {
                m_gpu_active_region_stop.at(domain_idx) = m_time.value;
                m_gpu_active_energy_stop.at(domain_idx) = m_gpu_energy.at(domain_idx).value;
            }
        }
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
bool GPUTorchAgent::do_write_batch(void) const
{
    return m_do_write_batch;
}

// Read signals from the platform and calculate samples to be sent up
void GPUTorchAgent::sample_platform(std::vector<double> &out_sample)
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
        m_gpu_energy.at(domain_idx).value = m_platform_io.sample(m_gpu_energy.at(
                                                                 domain_idx).batch_idx);
    }
    m_time.value = m_platform_io.sample(m_time.batch_idx);
}

// Wait for the remaining cycle time to keep Controller loop cadence
void GPUTorchAgent::wait(void)
{
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
    }
    while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
    geopm_time(&m_last_wait);
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > GPUTorchAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

// Adds number of frquency requests to the per-node section of the report
std::vector<std::pair<std::string, std::string> > GPUTorchAgent::report_host(void) const
{
    std::vector<std::pair<std::string, std::string> > result;

    result.push_back({"GPU Frequency Requests", std::to_string(m_gpu_frequency_requests)});

    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        double energy_stop = m_gpu_active_energy_stop.at(domain_idx);
        double energy_start = m_gpu_active_energy_start.at(domain_idx);
        double region_stop = m_gpu_active_region_stop.at(domain_idx);
        double region_start =  m_gpu_active_region_start.at(domain_idx);
        result.push_back({"GPU " + std::to_string(domain_idx) +
                          " Active Region Energy", std::to_string(energy_stop - energy_start)});
        result.push_back({"GPU " + std::to_string(domain_idx) +
                          " Active Region Time", std::to_string(region_stop - region_start)});
        // Region time is generally sufficient for non-debug cases
        result.push_back({"GPU " + std::to_string(domain_idx) +
                          " Active Region Start Time", std::to_string(region_start)});
        result.push_back({"GPU " + std::to_string(domain_idx) +
                          " Active Region Stop Time", std::to_string(region_stop)});
    }
    return result;
}

// This Agent does not add any per-region details
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > GPUTorchAgent::report_region(void) const
{
    return {};
}

// Adds trace columns signals of interest
std::vector<std::string> GPUTorchAgent::trace_names(void) const
{
    return {};
}

// Updates the trace with values for signals from this Agent
void GPUTorchAgent::trace_values(std::vector<double> &values)
{
}

void GPUTorchAgent::enforce_policy(const std::vector<double> &policy) const
{
}

std::vector<std::function<std::string(double)> > GPUTorchAgent::trace_formats(void) const
{
    return {};
}

// Name used for registration with the Agent factory
std::string GPUTorchAgent::plugin_name(void)
{
    return "gpu_torch";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> GPUTorchAgent::make_plugin(void)
{
    return geopm::make_unique<GPUTorchAgent>();
}

// Describes expected policies to be provided by the resource manager or user
std::vector<std::string> GPUTorchAgent::policy_names(void)
{
    return {"GPU_PHI"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> GPUTorchAgent::sample_names(void)
{
    return {};
}
