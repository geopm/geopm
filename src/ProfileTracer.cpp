/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "config.h"

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <iostream>
#include <iomanip>
#include "ProfileTracerImp.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "geopm_internal.h"
#include "Environment.hpp"
#include "Exception.hpp"
#include "CSV.hpp"
#include "geopm_debug.hpp"
#include "ApplicationSampler.hpp"
#include "record.hpp"

namespace geopm
{
    ProfileTracerImp::ProfileTracerImp()
        : ProfileTracerImp(1024 * 1024,
                           environment().do_trace_profile(),
                           environment().trace_profile(),
                           hostname(),
                           time_zero())
    {

    }

    ProfileTracerImp::ProfileTracerImp(size_t buffer_size,
                                       bool is_trace_enabled,
                                       const std::string &file_name,
                                       const std::string &host_name,
                                       const struct geopm_time_s &time_0)
        : m_is_trace_enabled(is_trace_enabled)
        , m_time_zero(time_0)
    {
        if (m_is_trace_enabled) {
            char time_cstr[NAME_MAX];
            int err = geopm_time_to_string(&m_time_zero, NAME_MAX, time_cstr);
            if (err) {
                throw Exception("geopm_time_to_string() failed",
                                err, __FILE__, __LINE__);
            }
            m_csv = geopm::make_unique<CSVImp>(file_name, host_name, time_cstr, buffer_size);

            m_csv->add_column("TIME", "double");
            m_csv->add_column("PROCESS", "integer");
            m_csv->add_column("EVENT", event_format);
            m_csv->add_column("SIGNAL", event_format);
            m_csv->activate();
        }
    }

    ProfileTracerImp::~ProfileTracerImp() = default;

    std::string ProfileTracerImp::event_format(double value)
    {
        static bool is_signal = false;
        static int event_type;
        std::string result;

        if (!is_signal) {
            // This is a call to format the event column
            // Store the event type for the next call
            event_type = value;
            result = event_name((int)value);
            // The next call will format the signal column
            is_signal = true;
        }
        else {
            // This is a call to format the signal column
            switch (event_type) {
                case EVENT_REGION_ENTRY:
                case EVENT_REGION_EXIT:
                    result = string_format_hex(value);
                    break;
                case EVENT_EPOCH_COUNT:
                    result = string_format_integer(value);
                    break;
                case EVENT_HINT:
                    result = hint_name(value);
                    break;
                default:
                    result = "INVALID";
                    GEOPM_DEBUG_ASSERT(false, "ProfileTracer::event_format(): event out of range");
                    break;
            }
            // The next call will be to format the event column
            is_signal = false;
        }
        return result;
    }

    void ProfileTracerImp::update(const std::vector<record_s> &records)
    {
        if (m_is_trace_enabled) {
            std::vector<double> sample(M_NUM_COLUMN);
            for (const auto &it : records) {
                sample[M_COLUMN_TIME] = it.time;
                sample[M_COLUMN_PROCESS] = it.process;
                sample[M_COLUMN_EVENT] = it.event;
                sample[M_COLUMN_SIGNAL] = it.signal;
                m_csv->update(sample);
            }
        }
    }

    std::unique_ptr<ProfileTracer> ProfileTracer::make_unique(void)
    {
        return geopm::make_unique<ProfileTracerImp>();
    }
}
