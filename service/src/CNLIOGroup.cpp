/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CNLIOGroup.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>

#include "geopm/Agg.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"

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
        : m_signal_available({{"CNL::BOARD_POWER", {
                                   "Point in time power",
                                   Agg::average,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/power", "W"),
                                   false,
                                   NAN,
                                   M_UNITS_WATTS,
                                   IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE}},
                              {"CNL::BOARD_ENERGY", {
                                   "Accumulated energy",
                                   Agg::sum,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/energy", "J"),
                                   false,
                                   NAN,
                                   M_UNITS_JOULES,
                                   IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE}},
                              {"CNL::POWER_MEMORY", {
                                   "Point in time memory power",
                                   Agg::average,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/memory_power", "W"),
                                   false,
                                   NAN,
                                   M_UNITS_WATTS,
                                   IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE}},
                              {"CNL::ENERGY_MEMORY", {
                                   "Accumulated memory energy",
                                   Agg::sum,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/memory_energy", "J"),
                                   false,
                                   NAN,
                                   M_UNITS_JOULES,
                                   IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE}},
                              {"CNL::BOARD_POWER_CPU", {
                                   "Point in time CPU power",
                                   Agg::average,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/cpu_power", "W"),
                                   false,
                                   NAN,
                                   M_UNITS_WATTS,
                                   IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE}},
                              {"CNL::BOARD_ENERGY_CPU", {
                                   "Accumulated CPU energy",
                                   Agg::sum,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/cpu_energy", "J"),
                                   false,
                                   NAN,
                                   M_UNITS_JOULES,
                                   IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE}},
                              {"CNL::SAMPLE_RATE", {
                                   "Sample frequency",
                                   Agg::expect_same,
                                   string_format_integer,
                                   std::bind(&CNLIOGroup::m_sample_rate, this),
                                   false,
                                   NAN,
                                   M_UNITS_HERTZ,
                                   IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT}},
                              {"CNL::SAMPLE_ELAPSED_TIME", {
                                   "Time that the sample was reported, in seconds since this agent initialized",
                                   Agg::max,
                                   string_format_double,
                                   std::bind(&CNLIOGroup::read_time, this, cpu_info_path + "/" + FRESHNESS_FILE_NAME),
                                   false,
                                   NAN,
                                   M_UNITS_SECONDS,
                                   IOGroup::M_SIGNAL_BEHAVIOR_MONOTONE}},
                             })
        , m_time_zero(geopm::time_zero())
    {
        m_sample_rate = read_double_from_file(
            cpu_info_path + "/" + RAW_SCAN_HZ_FILE_NAME, "");
        if (m_sample_rate <= 0) {
            throw Exception("CNLIOGroup::CNLIOGroup(): Unexpected sample frequency " +
                                std::to_string(m_sample_rate),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_initial_freshness =
            read_double_from_file(cpu_info_path + "/" + FRESHNESS_FILE_NAME, "");

        for (const auto &signal : m_signal_available) {
            // Attempt to call each of the read functions so we can fail
            // construction of this IOGroup if it isn't supported.
            signal.second.m_read_function();
        }

        register_signal_alias("BOARD_POWER", "CNL::BOARD_POWER");
        register_signal_alias("BOARD_ENERGY", "CNL::BOARD_ENERGY");
    }

    std::set<std::string> CNLIOGroup::signal_names(void) const
    {
        std::set<std::string> names;
        for (const auto &sv : m_signal_available) {
            names.insert(sv.first);
        }
        return names;
    }

    std::set<std::string> CNLIOGroup::control_names(void) const
    {
        return {};
    }

    bool CNLIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
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
        if (!is_valid_signal(signal_name)) {
            throw Exception("CNLIOGroup::push_signal(): " + signal_name +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("CNLIOGroup::push_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_signal_available[signal_name].m_do_read = true;
        return std::distance(m_signal_available.begin(), m_signal_available.find(signal_name));
    }

    int CNLIOGroup::push_control(const std::string &control_name,
                                 int domain_type, int domain_idx)
    {
        throw Exception("CNLIOGroup::push_control(): there are no controls supported by the CNLIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void CNLIOGroup::read_batch(void)
    {
        for (auto &signal : m_signal_available) {
            if (signal.second.m_do_read) {
                signal.second.m_value = signal.second.m_read_function();
            }
        }
    }

    void CNLIOGroup::write_batch(void) {}

    double CNLIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        auto res_it = m_signal_available.begin();
        if (batch_idx >= 0 && batch_idx < static_cast<int>(m_signal_available.size())) {
            std::advance(res_it, batch_idx);
            if(res_it->second.m_do_read) {
                result = res_it->second.m_value;
            }
            else if (!res_it->second.m_do_read) {
                throw Exception("CNLIOGroup::sample(): batch_idx " +
                                std::to_string(batch_idx) + " has not been pushed",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        else {
            throw Exception("CNLIOGroup::sample(): batch_idx " + std::to_string(batch_idx) +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
        if (!is_valid_signal(signal_name)) {
            throw Exception("CNLIOGroup::read_signal(): " + signal_name +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("CNLIOGroup:read_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.find(signal_name)->second.m_read_function();
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
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("CNLIOGroup::agg_function(): unknown how to aggregate \"" +
                            signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_agg_function;
    }

    std::function<std::string(double)>
    CNLIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("CNLIOGroup::format_function(): unknown how to format \"" +
                            signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_format_function;
    }

    std::string CNLIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CNLIOGroup::signal_description(): " + signal_name +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string result = "Invalid signal description: no description found.";
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result =  "    description: " + it->second.m_description + '\n'; // Includes alias_for if applicable
            result += "    units: " + IOGroup::units_to_string(it->second.m_units) + '\n';
            result += "    aggregation: " + Agg::function_to_name(it->second.m_agg_function) + '\n';
            result += "    domain: " + PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_BOARD) + '\n';
            result += "    iogroup: CNLIOGroup";
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("CNLIOGroup::signal_description(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string CNLIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception(
            "CNLIOGroup::control_description(): there are no controls "
            "supported by the CNLIOGroup",
            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int CNLIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CNLIOGroup::signal_behavior(): " + signal_name +
                            "not valid for CNLIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.m_behavior;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("CNLIOGroup::signal_description(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string CNLIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::string CNLIOGroup::plugin_name(void)
    {
        return "CNL";
    }

    std::unique_ptr<IOGroup> CNLIOGroup::make_plugin(void)
    {
        return geopm::make_unique<CNLIOGroup>();
    }

    double CNLIOGroup::read_time(const std::string &freshness_path) const
    {
        auto freshness = read_double_from_file(freshness_path, "");
        return (freshness - m_initial_freshness) / m_sample_rate;
    }

    void CNLIOGroup::register_signal_alias(const std::string &alias_name,
                                           const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("CNLIOGroup::register_signal_alias(): signal_name " + alias_name +
                            " was previously registered.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            // skip adding an alias if underlying signal is not found
            return;
        }
        // copy signal info but append to description
        m_signal_available[alias_name] = it->second;
        m_signal_available[alias_name].m_description =
            m_signal_available[signal_name].m_description + '\n' + "    alias_for: " + signal_name;
    }

    void CNLIOGroup::save_control(const std::string &save_path)
    {

    }

    void CNLIOGroup::restore_control(const std::string &save_path)
    {

    }
}
