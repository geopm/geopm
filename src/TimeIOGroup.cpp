/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "TimeIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
#include "config.h"

#define GEOPM_TIME_IO_GROUP_PLUGIN_NAME "TIME"

namespace geopm
{
    TimeIOGroup::TimeIOGroup()
        : m_is_signal_pushed(false)
        , m_is_batch_read(false)
        , m_valid_signal_name{plugin_name() + "::ELAPSED",
                              "TIME"}
    {
        geopm_time(&m_time_zero);
    }

    std::set<std::string> TimeIOGroup::signal_names(void) const
    {
        return m_valid_signal_name;
    }

    std::set<std::string> TimeIOGroup::control_names(void) const
    {
        return {};
    }

    bool TimeIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_valid_signal_name.find(signal_name) != m_valid_signal_name.end();
    }

    bool TimeIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int TimeIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = PlatformTopo::M_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = PlatformTopo::M_DOMAIN_BOARD;
        }
        return result;
    }

    int TimeIOGroup::control_domain_type(const std::string &control_name) const
    {
        return PlatformTopo::M_DOMAIN_INVALID;
    }

    int TimeIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::push_signal(): signal_name " + signal_name +
                            " not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_is_batch_read) {
            throw Exception("TimeIOGroup::push_signal(): cannot push signal after call to read_batch().",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_is_signal_pushed = true;
        return 0;
    }

    int TimeIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("TimeIOGroup::push_control(): there are no controls supported by the TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void TimeIOGroup::read_batch(void)
    {
        if (m_is_signal_pushed) {
            m_time_curr = geopm_time_since(&m_time_zero);
        }
        m_is_batch_read = true;
    }

    void TimeIOGroup::write_batch(void)
    {

    }

    double TimeIOGroup::sample(int batch_idx)
    {
        if (!m_is_signal_pushed) {
            throw Exception("TimeIOGroup::sample(): signal has not been pushed",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!m_is_batch_read) {
            throw Exception("TimeIOGroup::sample(): signal has not been read",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (batch_idx != 0) {
            throw Exception("TimeIOGroup::sample(): batch_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_time_curr;
    }

    void TimeIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("TimeIOGroup::adjust(): there are no controls supported by the TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double TimeIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup:read_signal(): " + signal_name +
                            "not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return geopm_time_since(&m_time_zero);
    }

    void TimeIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("TimeIOGroup::write_control(): there are no controls supported by the TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void TimeIOGroup::save_control(void)
    {

    }

    void TimeIOGroup::restore_control(void)
    {

    }

    std::string TimeIOGroup::plugin_name(void)
    {
        return GEOPM_TIME_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> TimeIOGroup::make_plugin(void)
    {
        return std::unique_ptr<IOGroup>(new TimeIOGroup);
    }

    std::function<double(const std::vector<double> &)> TimeIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::agg_function(): " + signal_name +
                            "not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return Agg::average;
    }

    std::string TimeIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::signal_description(): " + signal_name +
                            "not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return "Time in seconds since the IOGroup load.";
    }

    std::string TimeIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("TimeIOGroup::control_description(): there are no controls supported by the TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
}
