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

#include "NodeTorchAgent.hpp"

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
    geopm::agent_factory().register_plugin(NodeTorchAgent::plugin_name(),
                                           NodeTorchAgent::make_plugin,
                                           Agent::make_dictionary(NodeTorchAgent::policy_names(),
                                                                  NodeTorchAgent::sample_names()));
}


NodeTorchAgent::NodeTorchAgent()
    : NodeTorchAgent(geopm::platform_io(), geopm::platform_topo())
{

}

NodeTorchAgent::NodeTorchAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
    : m_platform_io(plat_io)
    , m_platform_topo(topo)
    , m_last_wait{{0, 0}}
    , M_WAIT_SEC(0.050) // 50ms Wait
    , M_POLICY_PHI_DEFAULT(0.5)
    , M_GPU_ACTIVITY_CUTOFF(0.05)
    , M_NUM_PACKAGE(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
    , M_NUM_GPU(m_platform_topo.num_domain(GEOPM_DOMAIN_GPU))
    , m_do_write_batch(false)
    , m_package_nn_path("cpu_control.pt")
    , m_gpu_nn_path("gpu_control.pt")
{
    geopm_time(&m_last_wait);
}

// Push signals and controls for future batch read/write
void NodeTorchAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    m_package_frequency_requests = 0;
    m_gpu_frequency_requests = 0;

    char* env_nn_path = getenv("GEOPM_CPU_NN_PATH");
    if (env_nn_path != NULL) {
        std::cout << "Loading ENV: " << std::string(env_nn_path) << std::endl;
        m_package_nn_path = env_nn_path;
    }
    else {
        std::cerr << "GEOPM_CPU_NN_PATH is NULL.  Attempting to load local cpu_control.pt" << std::endl;
    }

    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_package_neural_net.push_back(torch::jit::load(m_package_nn_path));
    }

    env_nn_path = getenv("GEOPM_GPU_NN_PATH");
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

void NodeTorchAgent::init_platform_io(void)
{
    m_cpu_max_freq = m_platform_io.read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);
    m_gpu_max_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);


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

    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_package_power.push_back({m_platform_io.push_signal("CPU_POWER",
                                   GEOPM_DOMAIN_PACKAGE,
                                   domain_idx), NAN});
        m_package_freq_status.push_back({m_platform_io.push_signal("CPU_FREQUENCY_STATUS",
                                         GEOPM_DOMAIN_PACKAGE,
                                         domain_idx), NAN});
        m_package_temperature.push_back({m_platform_io.push_signal("CPU_PACKAGE_TEMPERATURE",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), NAN});
        m_package_uncore_freq_status.push_back({m_platform_io.push_signal("CPU_UNCORE_FREQUENCY_STATUS",
                                                GEOPM_DOMAIN_PACKAGE,
                                                domain_idx), NAN});
        m_package_qm_rate.push_back({m_platform_io.push_signal("MSR::QM_CTR_SCALED_RATE",
                                     GEOPM_DOMAIN_PACKAGE,
                                     domain_idx), NAN});
        m_package_inst_retired.push_back({m_platform_io.push_signal("CPU_INSTRUCTIONS_RETIRED",
                                          GEOPM_DOMAIN_PACKAGE,
                                          domain_idx), NAN});
        m_package_cycles_unhalted.push_back({m_platform_io.push_signal("CPU_CYCLES_THREAD",
                                             GEOPM_DOMAIN_PACKAGE,
                                             domain_idx), NAN});
        m_package_energy.push_back({m_platform_io.push_signal("CPU_ENERGY",
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

    m_time = {m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0), NAN};

    //Configuration of QM_CTR must match QM_CTR config used for training data
    m_platform_io.write_control("MSR::PQR_ASSOC:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
    m_platform_io.write_control("MSR::QM_EVTSEL:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
    m_platform_io.write_control("MSR::QM_EVTSEL:EVENT_ID", GEOPM_DOMAIN_BOARD, 0, 2);
}

// Validate incoming policy and configure default policy requests.
void NodeTorchAgent::validate_policy(std::vector<double> &in_policy) const
{
    assert(in_policy.size() == M_NUM_POLICY);

    ///////////////////////
    //CPU POLICY CHECKING//
    ///////////////////////
    // If no phi value is provided assume the default behavior.
    if (std::isnan(in_policy[M_POLICY_CPU_PHI])) {
        in_policy[M_POLICY_CPU_PHI] = M_POLICY_PHI_DEFAULT;
    }

    if (in_policy[M_POLICY_CPU_PHI] < 0.0 ||
        in_policy[M_POLICY_CPU_PHI] > 1.0) {
        throw geopm::Exception("NodeTorchAgent::" + std::string(__func__) +
                               "(): POLICY_CPU_PHI value out of range: " +
                               std::to_string(in_policy[M_POLICY_CPU_PHI]) + ".",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

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
void NodeTorchAgent::split_policy(const std::vector<double>& in_policy,
                                std::vector<std::vector<double> >& out_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
}

// Indicate whether to send the policy down to children
bool NodeTorchAgent::do_send_policy(void) const
{
    return true;
}

void NodeTorchAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                    std::vector<double>& out_sample)
{

}

// Indicate whether to send samples up to the parent
bool NodeTorchAgent::do_send_sample(void) const
{
    return false;
}

void NodeTorchAgent::adjust_platform(const std::vector<double>& in_policy)
{
    assert(in_policy.size() == M_NUM_POLICY);

    m_do_write_batch = false;

    // Per Package freq
    std::vector<double> package_freq_request;
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        //Create an input tensor
        torch::Tensor xs = torch::tensor({{(double) m_package_power.at(domain_idx).value,
                                           (double) m_package_freq_status.at(domain_idx).value,
                                           (double) m_package_temperature.at(domain_idx).value,
                                           (double) m_package_uncore_freq_status.at(domain_idx).value,
                                           (double) m_package_qm_rate.at(domain_idx).value,
                                           (double) m_package_inst_retired.at(domain_idx).value / m_package_cycles_unhalted.at(domain_idx).value,
                                           (double) m_package_inst_retired.at(domain_idx).value / m_package_energy.at(domain_idx).value,
                                           (double) m_package_acnt.at(domain_idx).value / m_package_mcnt.at(domain_idx).value,
                                           (double) m_package_pcnt.at(domain_idx).value / m_package_mcnt.at(domain_idx).value,
                                           (double) m_package_pcnt.at(domain_idx).value / m_package_acnt.at(domain_idx).value,
                                           (double) in_policy[M_POLICY_CPU_PHI]}});

        //Push tensor into IValue vector
        std::vector<torch::jit::IValue> model_input;
        model_input.push_back(xs);

        //Evaluate
        at::Tensor output = m_package_neural_net.at(domain_idx).forward(model_input).toTensor();

        //Save recommendation and convert to SI units for later application
        package_freq_request.push_back(output[0].item<double>() * 1e9);
    }

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

    // set frequency control per package
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        //NAN --> Max Frequency
        if(std::isnan(package_freq_request.at(domain_idx))) {
            package_freq_request.at(domain_idx) = m_cpu_max_freq;
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

    // set frequency control per accelerator
    for (int domain_idx = 0; domain_idx < M_NUM_GPU; ++domain_idx) {
        //NAN --> Max Frequency
        if(std::isnan(gpu_freq_request.at(domain_idx))) {
            gpu_freq_request.at(domain_idx) = m_gpu_max_freq;
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
bool NodeTorchAgent::do_write_batch(void) const
{
    return m_do_write_batch;
}

// Read signals from the platform and calculate samples to be sent up
void NodeTorchAgent::sample_platform(std::vector<double> &out_sample)
{
    assert(out_sample.size() == M_NUM_SAMPLE);

    // Collect latest signal values
    for (int domain_idx = 0; domain_idx < M_NUM_PACKAGE; ++domain_idx) {
        m_package_power.at(domain_idx).value = m_platform_io.sample(m_package_power.at(domain_idx).batch_idx);
        m_package_freq_status.at(domain_idx).value = m_platform_io.sample(m_package_freq_status.at(domain_idx).batch_idx);
        m_package_temperature.at(domain_idx).value = m_platform_io.sample(m_package_temperature.at(domain_idx).batch_idx);
        m_package_uncore_freq_status.at(domain_idx).value = m_platform_io.sample(m_package_uncore_freq_status.at(domain_idx).batch_idx);
        m_package_qm_rate.at(domain_idx).value = m_platform_io.sample(m_package_qm_rate.at(domain_idx).batch_idx);
        m_package_cycles_unhalted.at(domain_idx).value = m_platform_io.sample(m_package_cycles_unhalted.at(domain_idx).batch_idx);
        m_package_inst_retired.at(domain_idx).value = m_platform_io.sample(m_package_inst_retired.at(domain_idx).batch_idx);
        m_package_energy.at(domain_idx).value = m_platform_io.sample(m_package_energy.at(domain_idx).batch_idx);
        m_package_acnt.at(domain_idx).value = m_platform_io.sample(m_package_acnt.at(domain_idx).batch_idx);
        m_package_mcnt.at(domain_idx).value = m_platform_io.sample(m_package_mcnt.at(domain_idx).batch_idx);
        m_package_pcnt.at(domain_idx).value = m_platform_io.sample(m_package_pcnt.at(domain_idx).batch_idx);
    }

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
void NodeTorchAgent::wait(void)
{
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
    }
    while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
    geopm_time(&m_last_wait);
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > NodeTorchAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

// Adds number of frquency requests to the per-node section of the report
std::vector<std::pair<std::string, std::string> > NodeTorchAgent::report_host(void) const
{
    std::vector<std::pair<std::string, std::string> > result;

    result.push_back({"Xeon Package Frequency Requests", std::to_string(m_package_frequency_requests)});
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
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > NodeTorchAgent::report_region(void) const
{
    return {};
}

// Adds trace columns signals of interest
std::vector<std::string> NodeTorchAgent::trace_names(void) const
{
    return {};
}

// Updates the trace with values for signals from this Agent
void NodeTorchAgent::trace_values(std::vector<double> &values)
{
}

void NodeTorchAgent::enforce_policy(const std::vector<double> &policy) const
{
}

std::vector<std::function<std::string(double)> > NodeTorchAgent::trace_formats(void) const
{
    return {};
}

// Name used for registration with the Agent factory
std::string NodeTorchAgent::plugin_name(void)
{
    return "node_torch";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> NodeTorchAgent::make_plugin(void)
{
    return geopm::make_unique<NodeTorchAgent>();
}

// Describes expected policies to be provided by the resource manager or user
std::vector<std::string> NodeTorchAgent::policy_names(void)
{
    return {"CPU_PHI", "GPU_PHI"};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> NodeTorchAgent::sample_names(void)
{
    return {};
}
