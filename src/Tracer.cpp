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

#include "Tracer.hpp"
#include "Exception.hpp"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

namespace geopm
{
    Tracer::Tracer()
        : m_is_trace_enabled(false)
        , m_do_pad(false)
        , m_time_zero({{0,0}})
    {
        geopm_time(&m_time_zero);
        char *trace_env = getenv("GEOPM_TRACE");
        if (trace_env) {
            char hostname[NAME_MAX];
            int err = gethostname(hostname, NAME_MAX);
            if (err) {
                throw Exception("Tracer::Tracer() gethostname() failed", err, __FILE__, __LINE__);
            }
            std::string output_path(std::string(trace_env)  + "-" + std::string(hostname) + ".log");
            m_stream.open(output_path);
            m_is_trace_enabled = true;
        }
    }

    Tracer::~Tracer()
    {
        if (m_do_pad) {
            struct geopm_policy_message_s zero_policy {0, 0, 0, 0.0};
            update(zero_policy);
        }
        if (m_is_trace_enabled) {
            m_stream.close();
        }
    }

    void Tracer::update(const std::vector <struct geopm_telemetry_message_s> &telemetry)
    {
        if (m_is_trace_enabled)
        {
            if (m_do_pad) {
                struct geopm_policy_message_s zero_policy {0, 0, 0, 0.0};
                update(zero_policy);
            }
            for (auto it = telemetry.begin(); it != telemetry.end(); ++it) {
                m_stream << (*it).region_id << " | "
                         << geopm_time_diff(&m_time_zero, &((*it).timestamp)) << " | ";
                for (int i = 0; i != GEOPM_NUM_TELEMETRY_TYPE; ++i) {
                    m_stream << (*it).signal[i] << " | ";
                }
            }
            m_do_pad = true;
        }
    }

    void Tracer::update(const struct geopm_policy_message_s &policy)
    {
        if (m_is_trace_enabled)
        {
            m_stream << policy.mode << " | "
                     << policy.flags << " | "
                     << policy.num_sample << " | "
                     << policy.power_budget << std::endl;
            m_do_pad = false;
        }
    }
}
