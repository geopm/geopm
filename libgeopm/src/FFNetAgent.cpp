/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "FFNetAgent.hpp"

#include <sstream>
#include <cmath>
#include <cassert>
#include <algorithm>
#include <fstream>

#include "geopm/PlatformIOProf.hpp"
#include "geopm/Waiter.hpp"
#include "geopm/Environment.hpp"
#include "geopm_debug.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

namespace geopm
{
    const std::map<geopm_domain_e, std::string> FFNetAgent::M_NNET_ENVNAME =
        {{GEOPM_DOMAIN_PACKAGE, "GEOPM_CPU_NN_PATH"},
         {GEOPM_DOMAIN_GPU, "GEOPM_GPU_NN_PATH"}};

    const std::map<geopm_domain_e, std::string> FFNetAgent::M_FREQMAP_ENVNAME =
        {{GEOPM_DOMAIN_PACKAGE, "GEOPM_CPU_FMAP_PATH"},
         {GEOPM_DOMAIN_GPU, "GEOPM_GPU_FMAP_PATH"}};

    const std::map<geopm_domain_e, std::string> FFNetAgent::M_MAX_FREQ_SIGNAL_NAME = 
        {{GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MAX_AVAIL"},
         {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MAX_AVAIL"}};

    const std::map<geopm_domain_e, std::string> FFNetAgent::M_MIN_FREQ_SIGNAL_NAME = 
        {{GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MIN_AVAIL"},
         {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MIN_AVAIL"}};

    const std::map<geopm_domain_e, std::string> FFNetAgent::M_MAX_FREQ_CONTROL_NAME = 
        {{GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MAX_CONTROL"},
         {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MAX_CONTROL"}};
    const std::map<geopm_domain_e, std::string> FFNetAgent::M_MIN_FREQ_CONTROL_NAME = 
        {{GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MIN_CONTROL"},
         {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MIN_CONTROL"}};

    const std::map<geopm_domain_e, std::string> FFNetAgent::M_TRACE_SUFFIX = 
        {{GEOPM_DOMAIN_PACKAGE, "_cpu_"},
         {GEOPM_DOMAIN_GPU, "_gpu_"}};


    FFNetAgent::FFNetAgent()
        : FFNetAgent(platform_io(), platform_topo(), {}, {},
                     Waiter::make_unique(environment().period(M_WAIT_SEC)))
    {

    }

    FFNetAgent::FFNetAgent(PlatformIO &plat_io,
                           const PlatformTopo &topo,
                           const std::map<std::pair<geopm_domain_e, int>,
                                          std::shared_ptr<DomainNetMap> > &net_map,
                           const std::map<geopm_domain_e,
                                          std::shared_ptr<RegionHintRecommender> > &freq_recommender,
                           std::shared_ptr<Waiter> waiter)
        : m_platform_io(plat_io)
        , m_do_write_batch(false)
        , m_perf_energy_bias(0.0)
        , m_waiter(std::move(waiter))
    {
        init_domain_indices(topo);

        if (freq_recommender.empty()) {
            for (geopm_domain_e domain_type : m_domain_types) {
                std::string fpath = get_env_value(M_FREQMAP_ENVNAME.at(domain_type));
                int min_freq = m_platform_io.read_signal(M_MIN_FREQ_SIGNAL_NAME.at(domain_type),
                                                         GEOPM_DOMAIN_BOARD, 0);
                int max_freq = m_platform_io.read_signal(M_MAX_FREQ_SIGNAL_NAME.at(domain_type),
                                                         GEOPM_DOMAIN_BOARD, 0);

                m_freq_recommender[domain_type] =
                    RegionHintRecommender::make_shared(fpath, min_freq, max_freq);
            }
        }
        else {
            m_freq_recommender = freq_recommender;
        }

        if (net_map.empty()) {
            for (const m_domain_key_s domain_key : m_domains) {
                m_net_map[domain_key] =
                    DomainNetMap::make_shared(get_env_value(M_NNET_ENVNAME.at(domain_key.type)),
                                              domain_key.type,
                                              domain_key.index);
            }
        }
        else {
            for (const m_domain_key_s domain_key : m_domains) {
                m_net_map[domain_key] = net_map.at(std::make_pair(domain_key.type,
                                                                  domain_key.index));
            }
        }
    }

    void FFNetAgent::init_domain_indices(const PlatformTopo &topo) {
        m_domain_types.push_back(GEOPM_DOMAIN_PACKAGE);
        if (topo.num_domain(GEOPM_DOMAIN_GPU) > 0) {
            m_domain_types.push_back(GEOPM_DOMAIN_GPU);
        }

        for (geopm_domain_e domain_type : m_domain_types) {
            int count = topo.num_domain(domain_type);
            for (int domain_index=0; domain_index < count; ++domain_index) {
                m_domains.push_back({domain_type, domain_index});
            }
        }
    }

    std::string FFNetAgent::plugin_name(void)
    {
        return "ffnet";
    }

    // Name used for registration with the Agent factory
    std::unique_ptr<Agent> FFNetAgent::make_plugin(void)
    {
        return geopm::make_unique<FFNetAgent>();
    }

    // Push signals and controls for future batch read/write
    void FFNetAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        for (const m_domain_key_s domain_key : m_domains) {
            m_freq_control[domain_key].min_idx = 
                m_platform_io.push_control(M_MIN_FREQ_CONTROL_NAME.at(domain_key.type),
                                           domain_key.type,
                                           domain_key.index);
            m_freq_control[domain_key].max_idx = 
                m_platform_io.push_control(M_MAX_FREQ_CONTROL_NAME.at(domain_key.type),
                                           domain_key.type,
                                           domain_key.index);
            m_freq_control[domain_key].last_value = NAN;
        }

        m_platform_io.write_control("MSR::PQR_ASSOC:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
        m_platform_io.write_control("MSR::QM_EVTSEL:RMID", GEOPM_DOMAIN_BOARD, 0, 0);
        m_platform_io.write_control("MSR::QM_EVTSEL:EVENT_ID", GEOPM_DOMAIN_BOARD, 0, 2);
    }

    // Validate incoming policy and configure default policy requests.
    void FFNetAgent::validate_policy(std::vector<double> &in_policy) const
    {
        if (in_policy.size() != M_NUM_POLICY) {
            throw Exception("FFNetAgent::" + std::string(__func__) +
                            "(): policy vector not correctly sized.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (is_all_nan(in_policy)) {
            // All-NAN policy may be received before the first policy
            /// @todo: in the future, this should not be accepted by this agent.
            in_policy[M_POLICY_PERF_ENERGY_BIAS] = 0;
            return;
        }
        if (!std::isnan(in_policy[M_POLICY_PERF_ENERGY_BIAS])) {
            if (in_policy[M_POLICY_PERF_ENERGY_BIAS] > 1.0||
                in_policy[M_POLICY_PERF_ENERGY_BIAS] < 0) {
                throw Exception("FFNetAgent::" + std::string(__func__) +
                                "(): PERF_ENERGY_BIAS is out of range (should be 0-1). (",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);

            }
        }
    }

    // Distribute incoming policy to children
    void FFNetAgent::split_policy(const std::vector<double> &in_policy,
                                  std::vector<std::vector<double> > &out_policy)
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

    void FFNetAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        if (!std::isnan(in_policy[M_POLICY_PERF_ENERGY_BIAS])) {
            m_perf_energy_bias = in_policy[M_POLICY_PERF_ENERGY_BIAS];
        }
        m_do_write_batch = false;

        for (const m_domain_key_s domain_key : m_domains) {
            double new_freq =
                m_freq_recommender[domain_key.type]->recommend_frequency(m_net_map[domain_key]->last_output(),
                                                                         m_perf_energy_bias);
            if (!std::isnan(new_freq) &&
                m_freq_control[domain_key].last_value != new_freq) {
                m_platform_io.adjust(m_freq_control[domain_key].min_idx,
                                     new_freq);
                m_platform_io.adjust(m_freq_control[domain_key].max_idx,
                                     new_freq);
                m_freq_control[domain_key].last_value = new_freq;
                m_do_write_batch = true;
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
        for (auto &kv : m_net_map) {
            kv.second->sample();
        }
    }

    // Wait for the remaining cycle time to keep Controller loop cadence
    void FFNetAgent::wait(void)
    {
        m_waiter->wait();
    }

    // Describes expected policies to be provided by the resource manager or user
    std::vector<std::string> FFNetAgent::policy_names(void)
    {
        return {"PERF_ENERGY_BIAS"};
    }
    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> FFNetAgent::sample_names(void)
    {
        return {};
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > FFNetAgent::report_header(void) const
    {
        return {{"Wait time (sec)", std::to_string(m_waiter->period())}};
    }

    std::vector<std::pair<std::string, std::string> > FFNetAgent::report_host(void) const
    {
        return {};
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > >
            FFNetAgent::report_region(void) const
    {
        return {};
    }

    // Adds trace columns signals of interest
    std::vector<std::string> FFNetAgent::trace_names(void) const
    {
        std::vector<std::string> tracelist;
        for (const m_domain_key_s domain_key : m_domains) {
            for (const std::string& trace_name : m_net_map.at(domain_key)->trace_names()) {
                tracelist.push_back(trace_name +
                                    M_TRACE_SUFFIX.at(domain_key.type) +
                                    std::to_string(domain_key.index));
            }
        }
        return tracelist;
    }

    std::vector<std::function<std::string(double)> > FFNetAgent::trace_formats(void) const
    {
        return {};
    }

    // Updates the trace with values for signals from this Agent
    void FFNetAgent::trace_values(std::vector<double> &values)
    {
        int vidx = 0;
        for (const auto &kv : m_net_map) {
            std::vector<double> domain_row = kv.second->trace_values();
            for (size_t idx=0; idx < domain_row.size(); ++vidx, ++idx) {
                values[vidx] = domain_row[idx];
            }
        }
    }

    void FFNetAgent::enforce_policy(const std::vector<double> &policy) const
    {
    }

    bool FFNetAgent::m_domain_key_s::operator<(const m_domain_key_s &other) const
    {
        return type < other.type || (type == other.type && index < other.index);
    }

    bool FFNetAgent::is_all_nan(const std::vector<double> &vec)
    {
        return std::all_of(vec.begin(), vec.end(),
                           [](double x) -> bool { return std::isnan(x); });
    }

    std::string FFNetAgent::get_env_value(const std::string &env_var)
    {
        std::string value = geopm::get_env(env_var);
        if (value.empty()) {
            throw Exception("FFNetAgent::" + std::string(__func__) +
                            "(): environment variable not set: " + env_var  + ".",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return value;
    }
}
