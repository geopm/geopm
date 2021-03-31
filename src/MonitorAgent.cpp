/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

    MonitorAgent::MonitorAgent(PlatformIO &plat_io, const PlatformTopo &topo)
        : m_last_wait(time_zero())
        , M_WAIT_SEC(0.005)
    {
        geopm_time(&m_last_wait);
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

    }

    void MonitorAgent::validate_policy(std::vector<double> &policy) const
    {

    }

    void MonitorAgent::split_policy(const std::vector<double> &in_policy,
                                    std::vector<std::vector<double> > &out_policy)
    {

    }

    bool MonitorAgent::do_send_policy(void) const
    {
        return false;
    }

    void MonitorAgent::aggregate_sample(const std::vector<std::vector<double> > &in_sample,
                                        std::vector<double> &out_sample)
    {

    }

    bool MonitorAgent::do_send_sample(void) const
    {
        return false;
    }

    void MonitorAgent::adjust_platform(const std::vector<double> &in_policy)
    {

    }

    bool MonitorAgent::do_write_batch(void) const
    {
        return false;
    }

    void MonitorAgent::sample_platform(std::vector<double> &out_sample)
    {

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
        return {};
    }

    std::vector<std::pair<std::string, std::string> > MonitorAgent::report_header(void) const
    {
        return {};
    }

    std::vector<std::pair<std::string, std::string> > MonitorAgent::report_host(void) const
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

    std::vector<std::function<std::string(double)> > MonitorAgent::trace_formats(void) const
    {
        return {};
    }

    void MonitorAgent::trace_values(std::vector<double> &values)
    {

    }

    void MonitorAgent::enforce_policy(const std::vector<double> &policy) const
    {

    }
}
