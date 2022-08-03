/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FixedFrequencyAgent.hpp"

#include <iostream>

#include <cmath>
#include <algorithm>

#include "geopm/PluginFactory.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "geopm_debug.hpp"

#include <string>

namespace geopm
{
    FixedFrequencyAgent::FixedFrequencyAgent():
        FixedFrequencyAgent(platform_io(), platform_topo())
    {
    }

    FixedFrequencyAgent::FixedFrequencyAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(platform_topo())
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(0.005) // 5mS wait default, override with policy parameter
        , m_is_adjust_initialized(false)
    {
        geopm_time(&m_last_wait);
    }

    // Push signals and controls for future batch read/write
    void FixedFrequencyAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
    }

    // Validate incoming policy and configure default policy requests.
    void FixedFrequencyAgent::validate_policy(std::vector<double> &in_policy) const
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY, "Incorrect policy size");
        double gpu_min_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
        double gpu_max_freq = m_platform_io.read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);
        double core_freq_min = m_platform_io.read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0);
        double core_freq_max = m_platform_io.read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0);

	if (!std::isnan(in_policy[M_POLICY_GPU_FREQUENCY])) {
	    if (in_policy[M_POLICY_GPU_FREQUENCY] > gpu_max_freq ||
	        in_policy[M_POLICY_GPU_FREQUENCY] < gpu_min_freq) {
	        throw Exception("FixedFrequenyAgent::" + std::string(__func__) +
                                "(): gpu frequency out of range: " +
                                std::to_string(in_policy[M_POLICY_GPU_FREQUENCY]) + ".",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
	  }
	}

	if (!std::isnan(in_policy[M_POLICY_CPU_FREQUENCY])) {
	    if (in_policy[M_POLICY_CPU_FREQUENCY] > core_freq_max ||
	        in_policy[M_POLICY_CPU_FREQUENCY] < core_freq_min) {
                throw Exception("FixedFrequencyAgent::" + std::string(__func__) +
                                "(): cpu frequency out of range: " +
                                std::to_string(in_policy[M_POLICY_CPU_FREQUENCY]) + ".",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
	  }
	}

	if (!std::isnan(in_policy[M_POLICY_UNCORE_MIN_FREQUENCY]) &&
	    !std::isnan(in_policy[M_POLICY_UNCORE_MAX_FREQUENCY])) {
	    if (in_policy[M_POLICY_UNCORE_MIN_FREQUENCY] > in_policy[M_POLICY_UNCORE_MAX_FREQUENCY]){
                throw Exception("FixedFrequencyAgent::" + std::string(__func__) +
                                "(): min uncore frequency cannot be larger than max uncore frequency: " +
                                std::to_string(in_policy[M_POLICY_UNCORE_MIN_FREQUENCY]) + " " +
                                std::to_string(in_policy[M_POLICY_UNCORE_MAX_FREQUENCY]) + ".",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
	    }
	}
	else if (!(std::isnan(in_policy[M_POLICY_UNCORE_MIN_FREQUENCY]) &&
		   std::isnan(in_policy[M_POLICY_UNCORE_MAX_FREQUENCY]))) {
	         throw Exception("FixedFrequencyAgent::" + std::string(__func__) +
                                 "(): when using NAN for uncore frequency, both min and max must be NAN: " +
                                 std::to_string(in_policy[M_POLICY_UNCORE_MIN_FREQUENCY]) + " " +
                                 std::to_string(in_policy[M_POLICY_UNCORE_MAX_FREQUENCY]) + ".",
                                 GEOPM_ERROR_INVALID, __FILE__, __LINE__);

	}

	if (!std::isnan(in_policy[M_POLICY_SAMPLE_PERIOD])) {
	    if (in_policy[M_POLICY_SAMPLE_PERIOD] <= 0.0) {
	        throw Exception("FixedFrequencyAgent::" + std::string(__func__) +
                                "(): sample period must be greater than 0: " +
			        std::to_string(in_policy[M_POLICY_CPU_FREQUENCY]) + ".",
			        GEOPM_ERROR_INVALID, __FILE__, __LINE__);

	  }
	}

    }

    // Distribute incoming policy to children
    void FixedFrequencyAgent::split_policy(const std::vector<double>& in_policy,
                                    std::vector<std::vector<double> >& out_policy)
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY, "Incorrect policy size");
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
    }

    // Indicate whether to send the policy down to children
    bool FixedFrequencyAgent::do_send_policy(void) const
    {
        return true;
    }

    void FixedFrequencyAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double>& out_sample)
    {

    }

    // Indicate whether to send samples up to the parent
    bool FixedFrequencyAgent::do_send_sample(void) const
    {
        return false;
    }

    void FixedFrequencyAgent::adjust_platform(const std::vector<double>& in_policy)
    {
        GEOPM_DEBUG_ASSERT(in_policy.size() == M_NUM_POLICY, "Incorrect policy size");

        if (!m_is_adjust_initialized) {
            double gpu_freq_request = in_policy[M_POLICY_GPU_FREQUENCY];
            double cpu_freq_request = in_policy[M_POLICY_CPU_FREQUENCY];
            double uncore_min_freq_request = in_policy[M_POLICY_UNCORE_MIN_FREQUENCY];
            double uncore_max_freq_request = in_policy[M_POLICY_UNCORE_MAX_FREQUENCY];
            double sample_period = in_policy[M_POLICY_SAMPLE_PERIOD];

            if (!std::isnan(sample_period)) {
                M_WAIT_SEC = sample_period;
            }

            // set gpu frequency control
            if (!std::isnan(gpu_freq_request)) {
                m_platform_io.write_control("GPU_CORE_FREQUENCY_CONTROL",GEOPM_DOMAIN_BOARD,0,gpu_freq_request);
            }

            // set CPU frequency control
            if (!std::isnan(cpu_freq_request)) {
                m_platform_io.write_control("CPU_FREQUENCY_CONTROL",GEOPM_DOMAIN_BOARD,0,cpu_freq_request);
            }

            // set Uncore frequency controls
            if (!std::isnan(uncore_min_freq_request)) {
                m_platform_io.write_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL",GEOPM_DOMAIN_BOARD,0,uncore_min_freq_request);
            }

            if (!std::isnan(uncore_max_freq_request)) {
                m_platform_io.write_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL",GEOPM_DOMAIN_BOARD,0,uncore_max_freq_request);
            }

            m_is_adjust_initialized = true;
        }

    }

    //If new values have been adjusted, write
    bool FixedFrequencyAgent::do_write_batch(void) const
    {
        return false;
    }

    // Read signals from the platform and calculate samples to be sent up
    void FixedFrequencyAgent::sample_platform(std::vector<double> &out_sample)
    {

    }

    // Wait for the remaining cycle time to keep Controller loop cadence at 1 second
    void FixedFrequencyAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    // Adds the wait time to the top of the report
    std::vector<std::pair<std::string, std::string> > FixedFrequencyAgent::report_header(void) const
    {
        return {};
    }

    // Adds to the per-node section of the report
    std::vector<std::pair<std::string, std::string> > FixedFrequencyAgent::report_host(void) const
    {
        return {};
    }

    // This Agent does not add any per-region details
    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > FixedFrequencyAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> FixedFrequencyAgent::trace_names(void) const
    {
        std::vector<std::string> names;
        return names;
    }

    // Updates the trace with values for samples and signals from this Agent
    void FixedFrequencyAgent::trace_values(std::vector<double> &values)
    {

    }

    std::vector<std::function<std::string(double)> > FixedFrequencyAgent::trace_formats(void) const
    {
        std::vector<std::function<std::string(double)>> trace_formats;
        return trace_formats;
    }

    // Name used for registration with the Agent factory
    std::string FixedFrequencyAgent::plugin_name(void)
    {
        return "fixed_frequency";
    }

    // Used by the factory to create objects of this type
    std::unique_ptr<Agent> FixedFrequencyAgent::make_plugin(void)
    {
        return geopm::make_unique<FixedFrequencyAgent>();
    }

    // Describes expected policies to be provided by the resource manager or user
    std::vector<std::string> FixedFrequencyAgent::policy_names(void)
    {
        return {"GPU_FREQUENCY",
                "CORE_FREQUENCY",
                "UNCORE_MIN_FREQUENCY",
                "UNCORE_MAX_FREQUENCY",
                "SAMPLE_PERIOD",
               };
    }

    // Describes samples to be provided to the resource manager or user
    std::vector<std::string> FixedFrequencyAgent::sample_names(void)
    {
        return {};
    }
}
