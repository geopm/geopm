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

#include <iomanip>
#include "ProfileTracer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "geopm_internal.h"
#include "geopm_env.h"

namespace geopm
{
    ProfileTracer::ProfileTracer()
        : ProfileTracer(geopm_env_do_trace(),
                       std::string(geopm_env_trace()) + "-profile-" + hostname(),
                       platform_io())
    {

    }


    ProfileTracer::ProfileTracer(bool do_trace, const std::string &file_name, IPlatformIO &platform_io)
        : m_do_trace(do_trace)
        , m_buffer_size(128 * 1024 * 1024)
        , m_platform_io(platform_io)
    {
        if (m_do_trace) {
            geopm_time(&m_time_zero);
            double rel_time = m_platform_io.read_signal("TIME", IPlatformTopo::M_DOMAIN_BOARD, 0);
            geopm_time_add(&m_time_zero, -rel_time, &m_time_zero);
            m_buffer.resize(m_buffer_size);
            m_stream.open(file_name);
            m_stream.rdbuf()->pubsetbuf(m_buffer.data(), m_buffer_size);
            m_stream << std::scientific;
            m_stream << "rank|region_id|timestamp|progress\n";
        }
    }

    void ProfileTracer::update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                              std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        if (m_do_trace) {
            for (auto it = prof_sample_begin; it != prof_sample_end; ++it) {
                m_stream << it->second.rank << "|"
                         << "0x" << std::hex << std::setfill('0') << std::setw(16) << std::fixed
                         << it->second.region_id << "|"
                         << std::setprecision(16) << std::scientific << std::dec
                         << geopm_time_diff(&m_time_zero, &(it->second.timestamp)) << "|"
                         << it->second.progress << "\n";
            }
        }
    }
}
