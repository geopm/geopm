/*
 * Copyright (c) 2016, Intel Corporation
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

#include <unistd.h>
#include <iomanip>
#include <sstream>

#include "Tracer.hpp"
#include "Exception.hpp"
#include "geopm_env.h"
#include "config.h"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

namespace geopm
{
    Tracer::Tracer(std::string header)
        : m_header(header)
        , m_is_trace_enabled(false)
        , m_do_header(true)
        , m_buffer_limit(134217728) // 128 MiB
        , m_time_zero({{0, 0}})
        , m_policy({0, 0, 0, 0.0})
    {
        geopm_time(&m_time_zero);
        if (geopm_env_do_trace()) {
            char hostname[NAME_MAX];
            int err = gethostname(hostname, NAME_MAX);
            if (err) {
                throw Exception("Tracer::Tracer() gethostname() failed", err, __FILE__, __LINE__);
            }
            m_hostname = hostname;
            std::ostringstream output_path;
            output_path << geopm_env_trace() << "-" << m_hostname;
            m_stream.open(output_path.str());
            m_buffer << std::setprecision(16);
            m_is_trace_enabled = true;
        }
    }

    Tracer::~Tracer()
    {
        m_stream << m_buffer.str();
        if (m_is_trace_enabled) {
            m_stream.close();
        }
    }

    void Tracer::update(const std::vector <struct geopm_telemetry_message_s> &telemetry)
    {
        if (m_is_trace_enabled && telemetry.size()) {
            if (m_do_header) {
                // Write the GlobalPolicy information first
                m_buffer << m_header;
                m_buffer << "# \"node_name\" : \"" << m_hostname << "\"" << std::endl;
                m_buffer << "region_id | seconds | ";
                for (size_t i = 0; i < telemetry.size(); ++i) {
                    m_buffer << "pkg_energy-" << i << " | "
                             << "dram_energy-" << i << " | "
                             << "frequency-" << i << " | "
                             << "inst_retired-" << i << " | "
                             << "clk_unhalted_core-" << i << " | "
                             << "clk_unhalted_ref-" << i << " | "
                             << "read_bandwidth-" << i << " | "
                             << "progress-" << i << " | "
                             << "runtime-" << i << " | ";
                }
                m_buffer << "policy_mode | policy_flags | policy_num_sample | policy_power_budget" << std::endl;
                m_do_header = false;
            }
            m_buffer << telemetry[0].region_id << " | "
                     << geopm_time_diff(&m_time_zero, &(telemetry[0].timestamp)) << " | ";
            for (auto it = telemetry.begin(); it != telemetry.end(); ++it) {
                for (int i = 0; i != GEOPM_NUM_TELEMETRY_TYPE; ++i) {
                    m_buffer << (*it).signal[i] << " | ";
                }
            }
            m_buffer << m_policy.mode << " | "
                     << m_policy.flags << " | "
                     << m_policy.num_sample << " | "
                     << m_policy.power_budget << std::endl;

        }
        if (m_buffer.tellp() > m_buffer_limit) {
            m_stream << m_buffer.str();
            m_buffer.str("");
        }
    }

    void Tracer::update(const struct geopm_policy_message_s &policy)
    {
        if (m_is_trace_enabled) {
            m_policy = policy;
        }
    }
}
