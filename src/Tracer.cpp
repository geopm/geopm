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

#include "Tracer.hpp"
#include "Exception.hpp"
#include "geopm_env.h"
#include "config.h"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

namespace geopm
{
    Tracer::Tracer()
        : m_is_trace_enabled(false)
        , m_do_header(true)
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
            std::string output_path(geopm_env_trace());
            output_path += "-" + std::string(hostname);
            m_stream.open(output_path);
            m_stream << std::setprecision(16);
            m_is_trace_enabled = true;
        }
    }

    Tracer::~Tracer()
    {
        if (m_is_trace_enabled) {
            m_stream.close();
        }
    }

    void Tracer::update(const std::vector <struct geopm_telemetry_message_s> &telemetry)
    {
        if (m_is_trace_enabled && telemetry.size()) {
            if (m_do_header) {
                m_stream << "region_id | seconds | ";
                for (size_t i = 0; i < telemetry.size(); ++i) {
                    m_stream << "pkg_energy-" + std::to_string(i) + " | dram_energy-" + std::to_string(i) + " | frequency-" + std::to_string(i) + " | inst_retired-" + std::to_string(i) + " | clk_unhalted_core-" + std::to_string(i) + " | clk_unhalted_ref-" + std::to_string(i) + " | read_bandwidth-" + std::to_string(i) + " | progress-" + std::to_string(i) + " | runtime-" + std::to_string(i) + " | ";
                }
                m_stream << "policy_mode | policy_flags | policy_num_sample | policy_power_budget" << std::endl;
                m_do_header = false;
            }
            m_stream << telemetry[0].region_id << " | "
                     << geopm_time_diff(&m_time_zero, &(telemetry[0].timestamp)) << " | ";
            for (auto it = telemetry.begin(); it != telemetry.end(); ++it) {
                for (int i = 0; i != GEOPM_NUM_TELEMETRY_TYPE; ++i) {
                    m_stream << (*it).signal[i] << " | ";
                }
            }
            m_stream << m_policy.mode << " | "
                     << m_policy.flags << " | "
                     << m_policy.num_sample << " | "
                     << m_policy.power_budget << std::endl;

        }
    }

    void Tracer::update(const struct geopm_policy_message_s &policy)
    {
        if (m_is_trace_enabled) {
            m_policy = policy;
        }
    }
}
