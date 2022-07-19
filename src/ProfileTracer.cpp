/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <string.h>
#include <errno.h>
#include <limits.h>
#include <iostream>
#include <iomanip>
#include "ProfileTracerImp.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm_hint.h"
#include "Environment.hpp"
#include "geopm/Exception.hpp"
#include "CSV.hpp"
#include "geopm_debug.hpp"
#include "ApplicationSampler.hpp"
#include "record.hpp"

namespace geopm
{
    // defintion of the static data member
    ApplicationSampler* ProfileTracerImp::m_application_sampler = nullptr;

    ProfileTracerImp::ProfileTracerImp(const std::string &start_time)
        : ProfileTracerImp(start_time,
                           1024 * 1024,
                           environment().do_trace_profile(),
                           environment().trace_profile(),
                           hostname(),
                           ApplicationSampler::application_sampler())
    {

    }

    ProfileTracerImp::ProfileTracerImp(const std::string &start_time,
                                       size_t buffer_size,
                                       bool is_trace_enabled,
                                       const std::string &file_name,
                                       const std::string &host_name,
                                       ApplicationSampler& application_sampler)
        : m_is_trace_enabled(is_trace_enabled)
    {
        m_application_sampler = &application_sampler;
        if (m_is_trace_enabled) {
            m_csv = geopm::make_unique<CSVImp>(file_name, host_name, start_time, buffer_size);

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
                case EVENT_SHORT_REGION:
                    GEOPM_DEBUG_ASSERT(m_application_sampler != nullptr,
                        "The ProfileTracerImp::ProfileTracerImp() must be called prior to calling ProfileTracerImp::event_format()");
                    result = string_format_hex(m_application_sampler->get_short_region(value).hash);
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

    std::unique_ptr<ProfileTracer> ProfileTracer::make_unique(const std::string &start_time)
    {
        return geopm::make_unique<ProfileTracerImp>(start_time);
    }
}
