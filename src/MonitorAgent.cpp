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
        , m_last_wait(GEOPM_TIME_REF)
        , m_num_ascend(0)
        , M_SEND_PERIOD(10)
        , M_WAIT_SEC(0.005)
    {
        geopm_time(&m_last_wait);

        for (auto name : sample_names()) {
            m_sample_idx.push_back(m_platform_io.push_signal(name,
                                                             IPlatformTopo::M_DOMAIN_BOARD,
                                                             0));
            m_agg_func.push_back(m_platform_io.agg_function(name));
        }
        m_num_sample = m_sample_idx.size();
    }

    std::string MonitorAgent::plugin_name(void)
    {
        return "monitor";
    }

    std::unique_ptr<Agent> MonitorAgent::make_plugin(void)
    {
        return geopm::make_unique<MonitorAgent>();
    }

    void MonitorAgent::init(int level, const std::vector<int> &fan_in, bool is_level_root)
    {
        m_level = level;
    }

    void MonitorAgent::validate_policy(std::vector<double> &in_policy) const
    {

    }

    bool MonitorAgent::descend(const std::vector<double> &in_policy,
                               std::vector<std::vector<double> >&out_policy)
    {
        return false;
    }

    bool MonitorAgent::ascend(const std::vector<std::vector<double> > &in_sample,
                              std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != m_num_sample) {
            throw Exception("MonitorAgent::ascend(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        aggregate_sample(in_sample, m_agg_func, out_sample);
        /// @todo should we check if the out_sample has changed before returning true?
        return true;
    }

    bool MonitorAgent::adjust_platform(const std::vector<double> &in_policy)
    {
        return false;
    }

    bool MonitorAgent::sample_platform(std::vector<double> &out_sample)
    {
#ifdef GEOPM_DEBUG
        if (out_sample.size() != m_num_sample) {
            throw Exception("MonitorAgent::sample_platform(): out_sample vector not correctly sized.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        bool result = false;
        if (m_num_ascend == 0) {
            for (size_t sample_idx = 0; sample_idx < m_num_sample; ++sample_idx) {
                out_sample[sample_idx] = m_platform_io.sample(m_sample_idx[sample_idx]);
            }
            result = true;
        }
        ++m_num_ascend;
        if (m_num_ascend == M_SEND_PERIOD) {
           m_num_ascend = 0;
        }
        return result;
    }

    void MonitorAgent::wait(void)
    {
        while(geopm_time_since(&m_last_wait) < M_WAIT_SEC) {

        }
        geopm_time(&m_last_wait);
    }

    std::vector<std::string> MonitorAgent::policy_names(void)
    {
        return {};
    }

    std::vector<std::string> MonitorAgent::sample_names(void)
    {
        return {"POWER_PACKAGE", "FREQUENCY"};
    }

    std::vector<std::pair<std::string, std::string> > MonitorAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > MonitorAgent::report_node(void) const
    {
        return {};
    }

    std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > MonitorAgent::report_region(void) const
    {
        return {};
    }

    std::vector<std::string> MonitorAgent::trace_names(void) const
    {
        return {};
    }

    void MonitorAgent::trace_values(std::vector<double> &values)
    {

    }
}
