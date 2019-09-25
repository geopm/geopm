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

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <iostream>
#include <iomanip>
#include "ProfileTracer.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "geopm_internal.h"
#include "Environment.hpp"
#include "Exception.hpp"
#include "CSV.hpp"

namespace geopm
{
    ProfileTracerImp::ProfileTracerImp()
        : ProfileTracerImp(1024 * 1024,
                           environment().do_trace_profile(),
                           environment().trace_profile(),
                           hostname(),
                           platform_io(),
                           GEOPM_TIME_REF)
    {

    }

    ProfileTracerImp::ProfileTracerImp(size_t buffer_size,
                                       bool is_trace_enabled,
                                       const std::string &file_name,
                                       const std::string &host_name,
                                       PlatformIO &platform_io,
                                       const struct geopm_time_s &time_zero)
        : m_is_trace_enabled(is_trace_enabled)
        , m_platform_io(platform_io)
        , m_time_zero(time_zero)
    {
        if (m_is_trace_enabled) {
            char time_cstr[NAME_MAX];
            int err = geopm_time_to_string(&time_zero, NAME_MAX, time_cstr);
            if (err) {
                throw Exception("geopm_time_to_string() failed",
                                err, __FILE__, __LINE__);
            }
            m_csv = geopm::make_unique<CSVImp>(file_name, host_name, time_cstr, buffer_size);

            if (geopm_time_diff(&m_time_zero, &GEOPM_TIME_REF) == 0.0) {
                geopm_time(&m_time_zero);
            }
            m_csv->add_column("RANK", "integer");
            m_csv->add_column("REGION_HASH", "hex");
            m_csv->add_column("REGION_HINT", "hex");
            m_csv->add_column("TIMESTAMP", "double");
            m_csv->add_column("PROGRESS", "float");
            m_csv->activate();

            double rel_time = m_platform_io.read_signal("TIME", GEOPM_DOMAIN_BOARD, 0);
            geopm_time_add(&m_time_zero, -rel_time, &m_time_zero);
        }
    }

    ProfileTracerImp::~ProfileTracerImp() = default;

    void ProfileTracerImp::update(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_begin,
                                  std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::const_iterator prof_sample_end)
    {
        if (m_is_trace_enabled) {
            std::vector<double> sample(M_NUM_COLUMN);
            for (auto it = prof_sample_begin; it != prof_sample_end; ++it) {
                sample[M_COLUMN_RANK] = it->second.rank;
                sample[M_COLUMN_REGION_HASH] = geopm_region_id_hash(it->second.region_id);
                sample[M_COLUMN_REGION_HINT] = geopm_region_id_hint(it->second.region_id);
                sample[M_COLUMN_TIME] = geopm_time_diff(&m_time_zero, &(it->second.timestamp));
                sample[M_COLUMN_PROGRESS] = it->second.progress;
                m_csv->update(sample);
            }
        }
    }
}
