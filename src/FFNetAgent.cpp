/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FFNetAgent.hpp"

#include <cmath>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

#include "PlatformIOProf.hpp"
#include "PluginFactory.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "Agg.hpp"

#include <string>

#define MAX_NNET_SIZE 1024*1024

using geopm::Agent;
using geopm::PlatformIO;
using geopm::PlatformTopo;

// Registers this Agent with the Agent factory, making it visible
// to the Controller when the plugin is first loaded.
static void __attribute__((constructor)) ffnet_agent_load(void)
{
    geopm::agent_factory().register_plugin(FFNetAgent::plugin_name(),
                                           FFNetAgent::make_plugin,
                                           Agent::make_dictionary(FFNetAgent::policy_names(),
                                                                  FFNetAgent::sample_names()));
}

FFNetAgent::FFNetAgent()
    : FFNetAgent(geopm::platform_io(), geopm::platform_topo())
{

}

FFNetAgent::FFNetAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
    : m_platform_io(plat_io)
    , m_platform_topo(topo)
    , m_last_wait{{0, 0}}
    , m_current_time{{0,0}}
    , m_time_delta(0.050)
    , M_WAIT_SEC(0.050) // 50ms Wait
    , M_NUM_PACKAGE(m_platform_topo.num_domain(GEOPM_DOMAIN_PACKAGE))
    , m_package_nn_path("region_id.pt")
{
    geopm_time(&m_last_wait);
    geopm_time(&m_current_time);
}

// Push signals and controls for future batch read/write
void FFNetAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
{
    char* env_nn_path = getenv("GEOPM_NN_PATH");
    if (env_nn_path != NULL) {
        std::cout << "Loading ENV: " << std::string(env_nn_path) << std::endl;
        m_package_nn_path = env_nn_path;
    }
    else {
        std::cerr << "GEOPM_NN_PATH is NULL.  Attempting to load local cpu_control.pt" << std::endl;
        // TODO actually load a backup neural net
        // maybe from a Json object that's built in?
    }

    std::ifstream file(m_package_nn_path);

    if (!file) {
        std::cerr << "Unable to open neural net file." << std::endl;
    }

    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    if (length >= MAX_NNET_SIZE) {
        std::cerr << "Neural net file exceeds maximum size." << std::endl;
    }

    std::string buf, err;
    buf.reserve(length);

    std::getline(file, buf);

    json11::Json nnet_json = json11::Json::parse(buf, err);

    m_package_neural_net = LocalNeuralNet(nnet_json["layers"]);

    init_platform_io(nnet_json);
}

void FFNetAgent::enforce_policy(const std::vector<double> &policy) const
{
}

void FFNetAgent::init_platform_io(json11::Json nnet_json)
{
    // TODO what does json11 do when the field is absent?
    for (int i=0; i<nnet_json["signal_inputs"].array_items().size(); i++) {
        m_signal_inputs.push_back({
                m_platform_io.push_signal(
                        nnet_json["signal_inputs"][i][0].string_value(),
                        nnet_json["signal_inputs"][i][1].int_value(),
                        nnet_json["signal_inputs"][i][2].int_value()),
                NAN});
    }

    for (int i=0; i<nnet_json["delta_inputs"].array_items().size(); i++) {
        m_delta_inputs.push_back({
                m_platform_io.push_signal(
                        nnet_json["delta_inputs"][i][0][0].string_value(),
                        nnet_json["delta_inputs"][i][0][1].int_value(),
                        nnet_json["delta_inputs"][i][0][2].int_value()),
                m_platform_io.push_signal(
                        nnet_json["delta_inputs"][i][1][0].string_value(),
                        nnet_json["delta_inputs"][i][1][1].int_value(),
                        nnet_json["delta_inputs"][i][1][2].int_value()),
                NAN, NAN, NAN, NAN});
    }

    for (int i=0; i<nnet_json["control_outputs"].array_items().size(); i++) {
        m_control_outputs.push_back(m_platform_io.push_control(
                        nnet_json["control_outputs"][i][0].string_value(),
                        nnet_json["control_outputs"][i][1].int_value(),
                        nnet_json["control_outputs"][i][2].int_value())
                        );
    }

    for (int i=0; i<nnet_json["trace_outputs"].array_items().size(); i++) {
        m_trace_outputs.push_back(nnet_json["trace_outputs"][i].string_value());
    }

    m_platform_io.write_control("MSR::PQR_ASSOC:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
    m_platform_io.write_control("MSR::QM_EVTSEL:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
    m_platform_io.write_control("MSR::QM_EVTSEL:EVENT_ID", GEOPM_DOMAIN_BOARD, 0, 2);
}

// Validate incoming policy and configure default policy requests.
void FFNetAgent::validate_policy(std::vector<double> &in_policy) const
{
}

// Distribute incoming policy to children
void FFNetAgent::split_policy(const std::vector<double>& in_policy,
                                std::vector<std::vector<double> >& out_policy)
{
    for (auto &child_pol : out_policy) {
        child_pol = in_policy;
    }
}

// Indicate whether to send the policy down to children
bool FFNetAgent::do_send_policy(void) const
{
    return true;
}

void FFNetAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                    std::vector<double>& out_sample)
{

}

// Indicate whether to send samples up to the parent
bool FFNetAgent::do_send_sample(void) const
{
    return false;
}

void FFNetAgent::adjust_platform(const std::vector<double>& in_policy)
{
    m_do_write_batch = true;

    for (int i=0; i<m_control_outputs.size(); i++) {
        if (std::isnan(m_last_output[i])) {
            m_do_write_batch = false;
        } else {
            m_platform_io.adjust(m_control_outputs[i], m_last_output[i]);
        }
    }
}

// If controls have a valid updated value write them.
bool FFNetAgent::do_write_batch(void) const
{
    return m_do_write_batch;
}

// Read signals from the platform and calculate samples to be sent up
void FFNetAgent::sample_platform(std::vector<double> &out_sample)
{
    m_sample ++;

    TensorOneD xs(m_signal_inputs.size() + m_delta_inputs.size());

    geopm_time_s last_time = m_current_time;
    geopm_time(&m_current_time);
    m_time_delta = geopm_time_diff(&last_time, &m_current_time);

    // Collect latest signal values
    for (int i=0; i<m_signal_inputs.size(); i++) {
        m_signal_inputs[i].signal = m_platform_io.sample(m_signal_inputs[i].batch_idx);
        xs[i] = m_platform_io.sample(m_signal_inputs[i].batch_idx);
    }
    for (int i=0; i<m_delta_inputs.size(); i++) {
        m_delta_inputs[i].signal_num_last = m_delta_inputs[i].signal_num;
        m_delta_inputs[i].signal_den_last = m_delta_inputs[i].signal_den;
        m_delta_inputs[i].signal_num = m_platform_io.sample(m_delta_inputs[i].batch_idx_num);
        m_delta_inputs[i].signal_den = m_platform_io.sample(m_delta_inputs[i].batch_idx_den);
        xs[m_signal_inputs.size() + i] =
            (m_delta_inputs[i].signal_num - m_delta_inputs[i].signal_num_last) /
                (m_delta_inputs[i].signal_den - m_delta_inputs[i].signal_den_last);
    }

    m_last_output = m_package_neural_net.model(xs);
}

// Wait for the remaining cycle time to keep Controller loop cadence
void FFNetAgent::wait(void)
{
    geopm_time_s current_time;
    do {
        geopm_time(&current_time);
    }
    while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
    geopm_time(&m_last_wait);
}

// Adds the wait time to the top of the report
std::vector<std::pair<std::string, std::string> > FFNetAgent::report_header(void) const
{
    return {{"Wait time (sec)", std::to_string(M_WAIT_SEC)}};
}

std::vector<std::pair<std::string, std::string> > FFNetAgent::report_host(void) const
{
    std::vector<std::pair<std::string, std::string> > result;
    return result;
}

// This Agent does not add any per-region details
std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > FFNetAgent::report_region(void) const
{
    return {};
}

// Adds trace columns signals of interest
std::vector<std::string> FFNetAgent::trace_names(void) const
{
    return m_trace_outputs;
}

// Updates the trace with values for signals from this Agent
void FFNetAgent::trace_values(std::vector<double> &values)
{
    if (m_last_output.get_dim() >= m_trace_outputs.size() + m_control_outputs.size()) {
        for (int i=0; i<values.size(); i++) {
            values[i] = m_last_output[i+m_control_outputs.size()];
        }
    }
}

std::vector<std::function<std::string(double)> > FFNetAgent::trace_formats(void) const
{
    return {};
}

// Name used for registration with the Agent factory
std::string FFNetAgent::plugin_name(void)
{
    return "ffnet";
}

// Used by the factory to create objects of this type
std::unique_ptr<Agent> FFNetAgent::make_plugin(void)
{
    return geopm::make_unique<FFNetAgent>();
}

// Describes expected policies to be provided by the resource manager or user
std::vector<std::string> FFNetAgent::policy_names(void)
{
    return {};
}

// Describes samples to be provided to the resource manager or user
std::vector<std::string> FFNetAgent::sample_names(void)
{
    return {};
}
