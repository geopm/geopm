/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FFNetAgent.hpp"

#include <sstream>
#include <cmath>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <fstream>

#include "PlatformIOProf.hpp"
#include "geopm_debug.hpp"
#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

#include <string>

using geopm::Agent;
using geopm::PlatformIO;
using geopm::PlatformTopo;

//TODO: Change from logits to probabilities
//
namespace geopm
{
    const std::map<geopm_domain_e, const char *> FFNetAgent::nnet_envname = 
            {
                {GEOPM_DOMAIN_PACKAGE, "GEOPM_CPU_NN_PATH"},
                {GEOPM_DOMAIN_GPU, "GEOPM_GPU_NN_PATH"}
            };

    const std::map<geopm_domain_e, const char *> FFNetAgent::freqmap_envname = 
            {
                {GEOPM_DOMAIN_PACKAGE, "GEOPM_CPU_FMAP_PATH"},
                {GEOPM_DOMAIN_GPU, "GEOPM_GPU_FMAP_PATH"}
            };

    const std::map<geopm_domain_e, std::string> FFNetAgent::max_freq_signal_name = 
            {
                {GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MAX_AVAIL"},
                {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MAX_AVAIL"}
            };

    const std::map<geopm_domain_e, std::string> FFNetAgent::min_freq_signal_name = 
            {
                {GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MIN_AVAIL"},
                {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MIN_AVAIL"}
            };

    const std::map<geopm_domain_e, std::string> FFNetAgent::max_freq_control_name = 
            {
                {GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MAX_CONTROL"},
                {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MAX_CONTROL"}
            };
    const std::map<geopm_domain_e, std::string> FFNetAgent::min_freq_control_name = 
            {
                {GEOPM_DOMAIN_PACKAGE, "CPU_FREQUENCY_MIN_CONTROL"},
                {GEOPM_DOMAIN_GPU, "GPU_CORE_FREQUENCY_MIN_CONTROL"}
            };

    const std::map<geopm_domain_e, std::string> FFNetAgent::trace_suffix = 
            {
                {GEOPM_DOMAIN_PACKAGE, "_cpu_"},
                {GEOPM_DOMAIN_GPU, "_gpu_"}
            };

    FFNetAgent::FFNetAgent()
        : FFNetAgent(geopm::platform_io(), geopm::platform_topo())
    {

    }

    FFNetAgent::FFNetAgent(
            geopm::PlatformIO &plat_io,
            const geopm::PlatformTopo &topo,
            std::map<std::pair<geopm_domain_e, int>, std::shared_ptr<DomainNetMap> > &net_map,
            std::map<geopm_domain_e, std::shared_ptr<RegionHintRecommender> > &freq_recommender
            )
        : m_platform_io(plat_io)
          , m_last_wait{{0, 0}}
          , M_WAIT_SEC(0.020) // 20ms Wait
          , m_perf_energy_bias(0)
    {
        geopm_time(&m_last_wait);
        init_domain_indices(topo);

        for (const geopm_domain_e domain_type : m_domain_types) {
            m_freq_recommender[domain_type] = freq_recommender.at(
                        domain_type
                    );
        }

        for (const m_domain_key_s domain_key : m_domains) {
            m_net_map[domain_key] = net_map.at(
                    std::make_pair(
                        domain_key.type,
                        domain_key.index)
                    );
        }
    }

    FFNetAgent::FFNetAgent(geopm::PlatformIO &plat_io, const geopm::PlatformTopo &topo)
        : m_platform_io(plat_io)
          , m_last_wait{{0, 0}}
          , M_WAIT_SEC(0.050) // 50ms Wait
          , m_perf_energy_bias(0)
    {
        geopm_time(&m_last_wait);
        init_domain_indices(topo);

        //Loading neural nets
        for (const m_domain_key_s domain_key : m_domains) {
            m_net_map[domain_key] = std::make_shared<DomainNetMapImp>(
                            getenv(nnet_envname.at(domain_key.type)),
                            domain_key.type,
                            domain_key.index);
        }

        for (geopm_domain_e domain_type : m_domain_types) {
            m_freq_recommender[domain_type] = std::make_shared<RegionHintRecommenderImp>(
                    getenv(freqmap_envname.at(domain_type)),
                    m_platform_io.read_signal(
                        min_freq_signal_name.at(domain_type),
                        GEOPM_DOMAIN_BOARD,
                        0),
                    m_platform_io.read_signal(
                        max_freq_signal_name.at(domain_type),
                        GEOPM_DOMAIN_BOARD,
                        0)
                    );
        }
    }

    void FFNetAgent::init_domain_indices(const geopm::PlatformTopo &topo) {
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
                    m_platform_io.push_control(
                        min_freq_control_name.at(domain_key.type),
                        domain_key.type,
                        domain_key.index);
            m_freq_control[domain_key].max_idx = 
                    m_platform_io.push_control(
                        max_freq_control_name.at(domain_key.type),
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
        if(in_policy.size() != M_NUM_POLICY) {
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
            if (in_policy[M_POLICY_PERF_ENERGY_BIAS] > 1.0 || in_policy[M_POLICY_PERF_ENERGY_BIAS] < 0) {
                throw Exception("FFNetAgent::" + std::string(__func__) +
                                "(): PERF_ENERGY_BIAS is out of range (should be 0-1). (",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);

            }
        }
    }

    bool FFNetAgent::is_all_nan(const std::vector<double> &vec)
    {
        return std::all_of(vec.begin(), vec.end(),
                           [](double x) -> bool { return std::isnan(x); });
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
        if (!std::isnan(in_policy[M_POLICY_PERF_ENERGY_BIAS])) {
            m_perf_energy_bias = in_policy[M_POLICY_PERF_ENERGY_BIAS];
        }
        m_do_write_batch = false;

        for (const m_domain_key_s domain_key : m_domains) {
            double new_freq = m_freq_recommender[domain_key.type]->recommend_frequency(
                    m_net_map[domain_key]->last_output(),
                    m_perf_energy_bias
                    );
            if (!std::isnan(new_freq)
                && m_freq_control[domain_key].last_value != new_freq) {
                m_platform_io.adjust(
                        m_freq_control[domain_key].min_idx,
                        new_freq
                        );
                m_platform_io.adjust(
                        m_freq_control[domain_key].max_idx,
                        new_freq
                        );
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
        ++m_sample;

        for (const m_domain_key_s domain_key : m_domains) {
            m_net_map[domain_key]->sample();
        }
    }

    // Wait for the remaining cycle time to keep Controller loop cadence
    void FFNetAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while (geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
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
        std::vector<std::string> tracelist;
        for (const m_domain_key_s domain_key : m_domains) {
            for (const std::string& trace_name : m_net_map.at(domain_key)->trace_names()) {
                tracelist.push_back(
                        trace_name
                        + trace_suffix.at(domain_key.type)
                        + std::to_string(domain_key.index));
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
        for (const m_domain_key_s domain_key : m_domains) {
            std::vector<double> domain_row = m_net_map[domain_key]->trace_values();
            for (std::size_t idx=0; idx < domain_row.size(); ++vidx, ++idx) {
                values[vidx] = domain_row[idx];
            }
        }
    }

    void FFNetAgent::enforce_policy(const std::vector<double> &policy) const
    {
    }
}
