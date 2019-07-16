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

#include "CNLIOGroup.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>

#include "Agg.hpp"
#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm_topo.h"

#include "config.h"

namespace geopm
{
    static const std::string FRESHNESS_FILE_NAME("freshness");
    static const std::string RAW_SCAN_HZ_FILE_NAME("raw_scan_hz");

    static std::function<double()> get_formatted_file_reader(const std::string &path,
                                                             const std::string &units)
    {
        return std::bind(read_double_from_file, path, units);
    }

    CNLIOGroup::CNLIOGroup()
        : CNLIOGroup("/sys/cray/pm_counters")
    {
    }

    CNLIOGroup::CNLIOGroup(const std::string &cpu_info_path)
        : m_signal_offsets{ { plugin_name() + "::POWER_BOARD", SIGNAL_TYPE_POWER_BOARD },
                            { "POWER_BOARD", SIGNAL_TYPE_POWER_BOARD },
                            { plugin_name() + "::ENERGY_BOARD", SIGNAL_TYPE_ENERGY_BOARD },
                            { "ENERGY_BOARD", SIGNAL_TYPE_ENERGY_BOARD },
                            { plugin_name() + "::POWER_BOARD_MEMORY",
                              SIGNAL_TYPE_POWER_MEMORY },
                            { "POWER_BOARD_MEMORY", SIGNAL_TYPE_POWER_MEMORY },
                            { plugin_name() + "::ENERGY_BOARD_MEMORY",
                              SIGNAL_TYPE_ENERGY_MEMORY },
                            { "ENERGY_BOARD_MEMORY", SIGNAL_TYPE_ENERGY_MEMORY },
                            { plugin_name() + "::POWER_BOARD_CPU", SIGNAL_TYPE_POWER_CPU },
                            { "POWER_BOARD_CPU", SIGNAL_TYPE_POWER_CPU },
                            { plugin_name() + "::ENERGY_BOARD_CPU", SIGNAL_TYPE_ENERGY_CPU },
                            { "ENERGY_BOARD_CPU", SIGNAL_TYPE_ENERGY_CPU },
                            { plugin_name() + "::SAMPLE_RATE", SIGNAL_TYPE_SAMPLE_RATE },
                            { plugin_name() + "::SAMPLE_ELAPSED_TIME",
                              SIGNAL_TYPE_ELAPSED_TIME } }
        , m_signals{
            { "Point in time board power, in Watts", Agg::average, string_format_integer,
              get_formatted_file_reader(cpu_info_path + "/power", "W"), false, NAN },
            { "Accumulated board energy, in Joules", Agg::sum, string_format_integer,
              get_formatted_file_reader(cpu_info_path + "/energy", "J"), false, NAN },
            { "Point in time memory power as seen from the board, in Watts",
              Agg::average, string_format_integer,
              get_formatted_file_reader(cpu_info_path + "/memory_power", "W"),
              false, NAN },
            { "Accumulated memory energy as seen from the board, in Joules",
              Agg::sum, string_format_integer,
              get_formatted_file_reader(cpu_info_path + "/memory_energy", "J"),
              false, NAN },
            { "Point in time cpu power as seen from the board, in Watts",
              Agg::average, string_format_integer,
              get_formatted_file_reader(cpu_info_path + "/cpu_power", "W"), false, NAN },
            { "Accumulated cpu energy as seen from the board, in Joules",
              Agg::sum, string_format_integer,
              get_formatted_file_reader(cpu_info_path + "/cpu_energy", "J"),
              false, NAN },
            { "Sample frequency, in Hertz", Agg::expect_same, string_format_integer,
              std::bind(&CNLIOGroup::m_sample_rate, this), false, NAN },
            { "Time that the sample was reported, in seconds since this agent initialized",
              Agg::max, string_format_double,
              std::bind(&CNLIOGroup::read_time, this, cpu_info_path + "/" + FRESHNESS_FILE_NAME),
              false, NAN }
        }
    {
        if (geopm_time(&m_time_zero)) {
            throw Exception("CNLIOGroup::CNLIOGroup(): Unable to get start time",
                            errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        m_sample_rate = read_double_from_file(
            cpu_info_path + "/" + RAW_SCAN_HZ_FILE_NAME, "");
        if (m_sample_rate <= 0) {
            throw Exception("CNLIOGroup::CNLIOGroup(): Unexpected sample frequency " +
                                std::to_string(m_sample_rate),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_initial_freshness =
            read_double_from_file(cpu_info_path + "/" + FRESHNESS_FILE_NAME, "");

        for (const auto &signal : m_signals) {
            // Attempt to call each of the read functions so we can fail
            // construction of this IOGroup if it isn't supported.
            signal.m_read_function();
        }
    }

    std::set<std::string> CNLIOGroup::signal_names(void) const
    {
        std::set<std::string> names;
        for (const auto &signal_offset_kv : m_signal_offsets) {
            names.insert(signal_offset_kv.first);
        }
        return names;
    }

    std::set<std::string> CNLIOGroup::control_names(void) const
    {
        return {};
    }

    bool CNLIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_offsets.find(signal_name) != m_signal_offsets.end();
    }

    bool CNLIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int CNLIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        return is_valid_signal(signal_name) ? GEOPM_DOMAIN_BOARD : GEOPM_DOMAIN_INVALID;
    }

    int CNLIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int CNLIOGroup::push_signal(const std::string &signal_name, int domain_type,
                                int domain_idx)
    {
        auto offset_it = m_signal_offsets.find(signal_name);
        if (offset_it == m_signal_offsets.end()) {
            throw Exception("CNLIOGroup::push_signal(): " + signal_name +
                                "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("CNLIOGroup::push_signal(): domain_type " +
                                std::to_string(domain_type) + "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_signals[offset_it->second].m_do_read = true;
        return offset_it->second;
    }

    int CNLIOGroup::push_control(const std::string &control_name,
                                 int domain_type, int domain_idx)
    {
        throw Exception("CNLIOGroup::push_control(): there are no controls supported by the CNLIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void CNLIOGroup::read_batch(void)
    {
        for (auto &signal : m_signals) {
            if (signal.m_do_read) {
                signal.m_value = signal.m_read_function();
            }
        }
    }

    void CNLIOGroup::write_batch(void) {}

    double CNLIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        if (batch_idx < 0 || batch_idx >= static_cast<int>(m_signals.size())) {
            throw Exception("CNLIOGroup::sample(): batch_idx " +
                                std::to_string(batch_idx) + " not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (!m_signals[batch_idx].m_do_read) {
            throw Exception("CNLIOGroup::sample(): batch_idx " +
                                std::to_string(batch_idx) + " has not been pushed",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else {
            result = m_signals[batch_idx].m_value;
        }
        return result;
    }

    void CNLIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("CNLIOGroup::adjust(): there are no controls supported by the CNLIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double CNLIOGroup::read_signal(const std::string &signal_name,
                                   int domain_type, int domain_idx)
    {
        auto offset_it = m_signal_offsets.find(signal_name);
        if (offset_it == m_signal_offsets.end()) {
            throw Exception("CNLIOGroup::read_signal(): " + signal_name +
                                "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("CNLIOGroup:read_signal(): domain_type " +
                                std::to_string(domain_type) + "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signals[offset_it->second].m_read_function();
    }

    void CNLIOGroup::write_control(const std::string &control_name,
                                   int domain_type, int domain_idx, double setting)
    {
        throw Exception(
            "CNLIOGroup::write_control(): there are no controls "
            "supported by the CNLIOGroup",
            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void CNLIOGroup::save_control(void) {}

    void CNLIOGroup::restore_control(void) {}

    std::function<double(const std::vector<double> &)>
    CNLIOGroup::agg_function(const std::string &signal_name) const
    {
        auto offset_it = m_signal_offsets.find(signal_name);
        if (offset_it == m_signal_offsets.end()) {
            throw Exception("CNLIOGroup::agg_function(): unknown how to aggregate \"" +
                                signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signals[offset_it->second].m_agg_function;
    }

    std::function<std::string(double)>
    CNLIOGroup::format_function(const std::string &signal_name) const
    {
        auto offset_it = m_signal_offsets.find(signal_name);
        if (offset_it == m_signal_offsets.end()) {
            throw Exception("CNLIOGroup::format_function(): unknown how to format \"" +
                                signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signals[offset_it->second].m_format_function;
    }

    std::string CNLIOGroup::signal_description(const std::string &signal_name) const
    {
        auto offset_it = m_signal_offsets.find(signal_name);
        if (offset_it == m_signal_offsets.end()) {
            throw Exception("CNLIOGroup::signal_description(): " + signal_name +
                                "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signals[offset_it->second].m_description;
    }

    std::string CNLIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception(
            "CNLIOGroup::control_description(): there are no controls "
            "supported by the CNLIOGroup",
            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    std::string CNLIOGroup::plugin_name(void)
    {
        return "CNL";
    }

    std::unique_ptr<IOGroup> CNLIOGroup::make_plugin(void)
    {
        return make_unique<CNLIOGroup>();
    }

    double CNLIOGroup::read_time(const std::string &freshness_path) const
    {
        auto freshness = read_double_from_file(freshness_path, "");
        return (freshness - m_initial_freshness) / m_sample_rate;
    }
}
