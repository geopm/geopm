/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "TimeIOGroup.hpp"

#include "geopm/PlatformTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "config.h"

#define GEOPM_TIME_IO_GROUP_PLUGIN_NAME "TIME"

namespace geopm
{
    TimeIOGroup::TimeIOGroup()
        : m_is_signal_pushed(false)
        , m_is_batch_read(false)
        , m_time_zero(time_zero())
        , m_time_curr(NAN)
        , m_valid_signal_name{plugin_name() + "::ELAPSED",
                              "TIME"}
    {

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
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = GEOPM_DOMAIN_CPU;
        }
        return result;
    }

    int TimeIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int TimeIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::push_signal(): signal_name " + signal_name +
                            " not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != GEOPM_DOMAIN_CPU) {
            throw Exception("TimeIOGroup::push_signal(): signal_name " + signal_name +
                            " not defined for domain " + std::to_string(domain_type),
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
            m_time_zero = geopm::time_zero();
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
        if (domain_type != GEOPM_DOMAIN_CPU) {
            throw Exception("TimeIOGroup::read_signal(): signal_name " + signal_name +
                            " not defined for domain " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_time_zero = geopm::time_zero();
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

    std::string TimeIOGroup::name(void) const
    {
        return plugin_name();
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
        return Agg::select_first;
    }

    std::function<std::string(double)> TimeIOGroup::format_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::format_function(): " + signal_name +
                            "not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return string_format_double;
    }

    std::string TimeIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::signal_description(): " + signal_name +
                            "not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string result = "Invalid signal description: no description found.";
        result = "    description: Time since the start of application profiling.\n";
        result += "    units: " + IOGroup::units_to_string(M_UNITS_SECONDS) + '\n';
        result += "    aggregation: " + Agg::function_to_name(Agg::select_first) + '\n';
        result += "    domain: " + platform_topo().domain_type_to_name(GEOPM_DOMAIN_CPU) + '\n';
        result += "    iogroup: TimeIOGroup";

        return result;
    }

    std::string TimeIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("TimeIOGroup::control_description(): there are no controls supported by the TimeIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int TimeIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("TimeIOGroup::signal_behavior(): " + signal_name +
                            "not valid for TimeIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE;
    }

    void TimeIOGroup::save_control(const std::string &save_path)
    {

    }

    void TimeIOGroup::restore_control(const std::string &save_path)
    {

    }
}
