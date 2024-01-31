/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "EndpointPolicyTracer.hpp"
#include "EndpointPolicyTracerImp.hpp"

#include <cstdlib>
#include <cstddef>
#include <climits>

#include "CSV.hpp"
#include "Agent.hpp"
#include "Environment.hpp"
#include "PlatformIOProf.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
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
                                  PlatformIOProf::platform_io(),
                                  Agent::policy_names(environment().agent()))
    {

    }

    EndpointPolicyTracerImp::EndpointPolicyTracerImp(size_t buffer_size,
                                                     bool is_trace_enabled,
                                                     const std::string &file_name,
                                                     PlatformIO &platform_io,
                                                     const std::vector<std::string> &policy_names)
        : m_is_trace_enabled(is_trace_enabled && policy_names.size() > 0)
        , m_platform_io(platform_io)
        , m_time_signal(-1)
        , m_num_policy(policy_names.size())
    {
        if (m_is_trace_enabled) {
            char time_cstr[NAME_MAX];
            geopm_time_s time_0 = time_zero();
            int err = geopm_time_to_string(&time_0, NAME_MAX, time_cstr);
            if (err) {
                throw Exception("geopm_time_to_string() failed",
                                err, __FILE__, __LINE__);
            }
            m_csv = geopm::make_unique<CSVImp>(file_name, "", time_cstr, buffer_size);

            m_csv->add_column("timestamp", "double");
            for (const auto &col : policy_names) {
                m_csv->add_column(col);
            }
            m_csv->activate();

            m_time_signal = m_platform_io.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0);

            m_values.resize(1 + m_num_policy);
        }
    }

    EndpointPolicyTracerImp::~EndpointPolicyTracerImp()
    {

    }

    void EndpointPolicyTracerImp::update(const std::vector<double> &policy)
    {
#ifdef GEOPM_DEBUG
        if (policy.size() != (size_t)m_num_policy) {
            throw Exception("EndpointPolicyTracerImp::update(): invalid policy size.",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        if (m_is_trace_enabled) {
            m_values[0] = m_platform_io.sample(m_time_signal);
            std::copy(policy.begin(), policy.end(), m_values.begin() + 1);
            m_csv->update(m_values);
        }
    }
}
