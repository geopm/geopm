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

#include "MonitorAgent.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    MonitorAgent::MonitorAgent()
        : MonitorAgent(platform_io(), platform_topo())
    {

    }

    MonitorAgent::MonitorAgent(IPlatformIO &plat_io, IPlatformTopo &topo)
        : m_platform_io(plat_io)
        , m_platform_topo(topo)
        , m_last_wait{{0, 0}}
    {
        load_trace_columns();

        // All columns sampled will be in the trace
        for (auto col : trace_columns()) {
            m_sample_idx.push_back(m_platform_io.push_signal(col.name,
                                                             col.domain_type,
                                                             col.domain_idx));
            m_agg_func.push_back(m_platform_io.agg_function(col.name));
        }
        m_num_sample = m_sample_idx.size();
    }

    std::string MonitorAgent::plugin_name(void)
    {
        return "monitor";
    }

    std::unique_ptr<IAgent> MonitorAgent::make_plugin(void)
    {
        return geopm::make_unique<MonitorAgent>();
    }

    void MonitorAgent::init(int level)
    {
        m_level = level;
    }

    void MonitorAgent::descend(const std::vector<double> &in_policy,
                                    std::vector<std::vector<double> >&out_policy)
    {

    }

    void MonitorAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                                   std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != m_num_sample) {
            throw Exception("MonitorAgent::ascend(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        std::vector<double> child_sample(in_sample.size());
        for (size_t sig_idx = 0; sig_idx < m_num_sample; ++sig_idx) {
            for (size_t child_idx = 0; child_idx < in_sample.size(); ++child_idx) {
                child_sample[child_idx] = in_sample[child_idx][sig_idx];
            }
            out_sample[sig_idx] = m_agg_func[sig_idx](child_sample);
        }
    }

    void MonitorAgent::adjust_platform(const std::vector<double> &in_policy)
    {

    }

    void MonitorAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != m_num_sample) {
            throw Exception("MonitorAgent::sample_platform(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        for (size_t sample_idx = 0; sample_idx < m_num_sample; ++sample_idx) {
            out_sample[sample_idx] = m_platform_io.sample(m_sample_idx[sample_idx]);
        }
    }

    void MonitorAgent::wait(void)
    {
        static double M_WAIT_SEC = 0.005;
        geopm_time_s current_time;
        geopm_time(&current_time);
        while(geopm_time_diff(&m_last_wait, &current_time) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> MonitorAgent::policy_names(void)
    {
        return {};
    }

    std::vector<std::string> MonitorAgent::sample_names(void)
    {
        return {"TIME", "POWER_PACKAGE", "FREQUENCY", "REGION_PROGRESS"};
    }

    std::string MonitorAgent::report_header(void)
    {
        return "";
    }

    std::string MonitorAgent::report_node(void)
    {
        return "";
    }

    std::map<uint64_t, std::string> MonitorAgent::report_region(void)
    {
        return {};
    }

    std::vector<IPlatformIO::m_request_s> MonitorAgent::trace_columns(void)
    {
        return m_trace_columns;
    }

    void MonitorAgent::load_trace_columns()
    {
        static std::vector<IPlatformIO::m_request_s> columns = {
            {"TIME", IPlatformTopo::M_DOMAIN_BOARD, 0},
            {"POWER_PACKAGE", IPlatformTopo::M_DOMAIN_BOARD, 0},
            {"FREQUENCY", IPlatformTopo::M_DOMAIN_BOARD, 0},
            {"REGION_PROGRESS", IPlatformTopo::M_DOMAIN_BOARD, 0},
        };

        const char* env_monitor_col_str = getenv("GEOPM_MONITOR_AGENT_SIGNALS");
        if (env_monitor_col_str) {
            std::string signals(env_monitor_col_str);
            std::string request;
            // split on comma
            size_t begin = 0;
            size_t end = -1;
            do {
                begin = end + 1;
                end = signals.find(",", begin);
                request = signals.substr(begin, end - begin);
                if (!request.empty()) {
                    m_trace_columns.push_back({request, IPlatformTopo::M_DOMAIN_BOARD, 0});
                }
            }
            while (end != std::string::npos);
        }
        else {
            m_trace_columns = columns;
        }
    }
}
