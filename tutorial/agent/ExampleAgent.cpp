/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "ExampleAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    ExampleAgent::ExampleAgent()
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_signal_idx(M_NUM_PLAT_SIGNAL)
        , m_control_idx(M_NUM_PLAT_CONTROL)
        , m_num_ascend(0)
        , M_SEND_PERIOD(10)
        , m_last_wait{{0, 0}}
        , M_WAIT_SEC(1.0)
        , m_min_idle(100.0)
        , m_max_idle(0.0)
    {
        geopm_time(&m_last_wait);
    }

    void ExampleAgent::init(int level)
    {
        // all signals and controls will be at board domain
        int board = IPlatformTopo::M_DOMAIN_BOARD;
        // push signals
        m_signal_idx[M_PLAT_SIGNAL_USER] = m_platform_io.push_signal("USER_TIME", board, 0);
        m_signal_idx[M_PLAT_SIGNAL_SYSTEM] = m_platform_io.push_signal("SYSTEM_TIME", board, 0);
        m_signal_idx[M_PLAT_SIGNAL_IDLE] = m_platform_io.push_signal("IDLE_TIME", board, 0);
        m_signal_idx[M_PLAT_SIGNAL_NICE] = m_platform_io.push_signal("NICE_TIME", board, 0);
        // push controls
        m_control_idx[M_PLAT_CONTROL_STDOUT] = m_platform_io.push_control("STDOUT", board, 0);
        m_control_idx[M_PLAT_CONTROL_STDERR] = m_platform_io.push_control("STDERR", board, 0);
    }

    bool ExampleAgent::descend(const std::vector<double> &in_policy,
                               std::vector<std::vector<double> >&out_policy)
    {
        for (auto &child_pol : out_policy) {
            child_pol = in_policy;
        }
        return true;
    }

    bool ExampleAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                              std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("ExampleAgent::ascend(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        std::vector<double> child_sample(in_sample.size());
        for (size_t sample_idx = 0; sample_idx < M_NUM_SAMPLE; ++sample_idx) {
            for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
                child_sample[child_idx] = in_sample[child_idx][sample_idx];
            }
            out_sample[sample_idx] = IPlatformIO::agg_average(child_sample);
        }
        return true;
    }

    bool ExampleAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        double idle_percent = m_last_sample[M_SAMPLE_IDLE_PCT];
        if (idle_percent < m_last_policy[M_POLICY_LOW_THRESH]) {
            m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL_STDERR], idle_percent);
        }
        else if (idle_percent > m_last_policy[M_POLICY_HIGH_THRESH]) {
            m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL_STDERR], idle_percent);
        }
        else {
            m_platform_io.adjust(m_control_idx[M_PLAT_CONTROL_STDOUT], idle_percent);
        }
        return true;
    }

    bool ExampleAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != M_NUM_SAMPLE) {
            throw Exception("ExampleAgent::sample_platform(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        // Collect latest times from platform signals
        double total = 0.0;
        for (auto signal_idx : m_signal_idx) {
            m_last_signal[signal_idx] = m_platform_io.sample(signal_idx);
            total += m_last_signal[signal_idx];
        }
        // Update samples
        m_last_sample[M_SAMPLE_USER_PCT] = m_last_signal[M_PLAT_SIGNAL_USER] / total;
        m_last_sample[M_SAMPLE_SYSTEM_PCT] = m_last_signal[M_PLAT_SIGNAL_SYSTEM] / total;
        m_last_sample[M_SAMPLE_IDLE_PCT] = m_last_signal[M_PLAT_SIGNAL_IDLE] / total;
        out_sample[M_SAMPLE_USER_PCT] = m_last_sample[M_SAMPLE_USER_PCT];
        out_sample[M_SAMPLE_SYSTEM_PCT] = m_last_sample[M_SAMPLE_SYSTEM_PCT];
        out_sample[M_SAMPLE_IDLE_PCT] = m_last_sample[M_SAMPLE_IDLE_PCT];
        return true;
    }

    void ExampleAgent::wait(void)
    {
        geopm_time_s current_time;
        do {
            geopm_time(&current_time);
        }
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC);
        geopm_time(&m_last_wait);
    }

    std::vector<std::pair<std::string, std::string> > ExampleAgent::report_header(void)
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > ExampleAgent::report_node(void)
    {
        return {
            {"Lowest idle %", std::to_string(m_min_idle)},
            {"Highest idle %", std::to_string(m_max_idle)}
        };
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > ExampleAgent::report_region(void)
    {
        return {};
    }

    std::vector<std::string> ExampleAgent::trace_names(void) const
    {
        return {"user_percent", "system_percent", "idle_percent",
                "user", "system", "idle", "nice"};
    }

    void ExampleAgent::trace_values(std::vector<double> &values)
    {
        // Sample values generated at last call to sample_platform
        values[M_SAMPLE_USER_PCT] = m_last_sample[M_SAMPLE_USER_PCT];
        values[M_SAMPLE_SYSTEM_PCT] = m_last_sample[M_SAMPLE_SYSTEM_PCT];
        values[M_SAMPLE_IDLE_PCT] = m_last_sample[M_SAMPLE_IDLE_PCT];
        // Signals measured at last call to sample_platform()
        values[M_PLAT_SIGNAL_USER] = m_last_signal[M_PLAT_SIGNAL_USER];
        values[M_PLAT_SIGNAL_SYSTEM] = m_last_signal[M_PLAT_SIGNAL_SYSTEM];
        values[M_PLAT_SIGNAL_IDLE] = m_last_signal[M_PLAT_SIGNAL_IDLE];
        values[M_PLAT_SIGNAL_NICE] = m_last_signal[M_PLAT_SIGNAL_NICE];
    }

    std::string ExampleAgent::plugin_name(void)
    {
        return "example";
    }

    std::unique_ptr<Agent> ExampleAgent::make_plugin(void)
    {
        return geopm::make_unique<ExampleAgent>();
    }

    std::vector<std::string> ExampleAgent::policy_names(void)
    {
        return {"LOW_THRESHOLD", "HIGH_THRESHOLD"};
    }

    std::vector<std::string> ExampleAgent::sample_names(void)
    {
        return {"USER_PERCENT", "SYSTEM_PERCENT", "IDLE_PERCENT"};
    }
}
