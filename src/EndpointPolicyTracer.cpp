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

#include "EndpointPolicyTracer.hpp"

#include <cstdlib>
#include <cstddef>
#include <climits>

#include "CSV.hpp"
#include "Agent.hpp"
#include "Environment.hpp"
#include "Helper.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "geopm_time.h"

#include "config.h"

namespace geopm
{
    std::unique_ptr<EndpointPolicyTracer> EndpointPolicyTracer::make_unique(void)
    {
        return geopm::make_unique<EndpointPolicyTracerImp>();
    }

    EndpointPolicyTracerImp::EndpointPolicyTracerImp()
        : EndpointPolicyTracerImp(1024 * 1024 * sizeof(char),
                                  environment().do_trace_endpoint_policy(),
                                  environment().trace_endpoint_policy(),
                                  platform_io(),
                                  Agent::policy_names(agent_factory().dictionary(environment().agent())))
    {

    }

    EndpointPolicyTracerImp::EndpointPolicyTracerImp(size_t buffer_size,
                                                     bool is_trace_enabled,
                                                     const std::string &file_name,
                                                     PlatformIO &platform_io,
                                                     const std::vector<std::string> &policy_names)
        : m_is_trace_enabled(is_trace_enabled)
        , m_platform_io(platform_io)
        , m_time_signal(-1)
    {
        if (m_is_trace_enabled) {
            char time_cstr[NAME_MAX];
            geopm_time_s time_zero;
            geopm_time(&time_zero);
            int err = geopm_time_to_string(&time_zero, NAME_MAX, time_cstr);
            if (err) {
                throw Exception("geopm_time_to_string() failed",
                                err, __FILE__, __LINE__);
            }
            m_csv = geopm::make_unique<CSVImp>(file_name, "", time_cstr, buffer_size);

            m_csv->add_column("timestamp", "double");
            for (auto col : policy_names) {
                m_csv->add_column(col);
            }
            m_csv->activate();

            m_time_signal = m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0);

            m_values.resize(1 + policy_names.size());
        }
    }

    EndpointPolicyTracerImp::~EndpointPolicyTracerImp()
    {

    }

    void EndpointPolicyTracerImp::update(const std::vector<double> &policy)
    {
        if (m_is_trace_enabled) {
            m_values[0] = m_platform_io.sample(m_time_signal);
            std::copy(policy.begin(), policy.end(), m_values.begin() + 1);
            m_csv->update(m_values);
        }
    }
}
