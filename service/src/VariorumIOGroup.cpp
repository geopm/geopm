/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "VariorumIOGroup.hpp"

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

    VariorumIOGroup::VariorumIOGroup()
        : VariorumIOGroup("Variorum")
    {
    }

    VariorumIOGroup::VariorumIOGroup(const std::string &cpu_info_path)
        : m_signal_available({{"Variorum::CPU_POWER", {
                                   "Point in time power",
                                   Agg::sum,
                                   string_format_integer,
                                   get_formatted_file_reader(cpu_info_path + "/power", "W"),
                                   false,
                                   NAN,
                                   M_UNITS_WATTS,
                                   IOGroup::M_SIGNAL_BEHAVIOR_VARIABLE}},
                             })
        , m_time_zero(geopm::time_zero())
    {
        m_sample_rate = read_double_from_file(
            cpu_info_path + "/" + RAW_SCAN_HZ_FILE_NAME, "");
        if (m_sample_rate <= 0) {
            throw Exception("VariorumIOGroup::VariorumIOGroup(): Unexpected sample frequency " +
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

        register_signal_alias("BOARD_POWER", "Variorum::BOARD_POWER");
        register_signal_alias("BOARD_ENERGY", "Variorum::BOARD_ENERGY");
    }

    std::set<std::string> VariorumIOGroup::signal_names(void) const
    {
        std::set<std::string> names;
        for (const auto &sv : m_signal_available) {
            names.insert(sv.first);
        }
        return names;
    }

    std::set<std::string> VariorumIOGroup::control_names(void) const
    {
        return {};
    }

    bool VariorumIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    bool VariorumIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int VariorumIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        return is_valid_signal(signal_name) ? GEOPM_DOMAIN_BOARD : GEOPM_DOMAIN_INVALID;
    }

    int VariorumIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int VariorumIOGroup::push_signal(const std::string &signal_name, int domain_type,
                                int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("VariorumIOGroup::push_signal(): " + signal_name +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("VariorumIOGroup::push_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_signal_available[signal_name].m_do_read = true;
        return std::distance(m_signal_available.begin(), m_signal_available.find(signal_name));
    }

    int VariorumIOGroup::push_control(const std::string &control_name,
                                 int domain_type, int domain_idx)
    {
        throw Exception("VariorumIOGroup::push_control(): there are no controls supported by the VariorumIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void VariorumIOGroup::read_batch(void)
    {
        for (auto &signal : m_signal_available) {
            if (signal.second.m_do_read) {
                signal.second.m_value = signal.second.m_read_function();
            }
        }
    }

    void VariorumIOGroup::write_batch(void) {}

    double VariorumIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        auto res_it = m_signal_available.begin();
        if (batch_idx >= 0 && batch_idx < static_cast<int>(m_signal_available.size())) {
            std::advance(res_it, batch_idx);
            if(res_it->second.m_do_read) {
                result = res_it->second.m_value;
            }
            else if (!res_it->second.m_do_read) {
                throw Exception("VariorumIOGroup::sample(): batch_idx " +
                                std::to_string(batch_idx) + " has not been pushed",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        else {
            throw Exception("VariorumIOGroup::sample(): batch_idx " + std::to_string(batch_idx) +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    void VariorumIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("VariorumIOGroup::adjust(): there are no controls supported by the VariorumIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double VariorumIOGroup::read_signal(const std::string &signal_name,
                                   int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("VariorumIOGroup::read_signal(): " + signal_name +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("VariorumIOGroup:read_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.find(signal_name)->second.m_read_function();
    }

    void VariorumIOGroup::write_control(const std::string &control_name,
                                   int domain_type, int domain_idx, double setting)
    {
        throw Exception(
            "VariorumIOGroup::write_control(): there are no controls "
            "supported by the VariorumIOGroup",
            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void VariorumIOGroup::save_control(void) {}

    void VariorumIOGroup::restore_control(void) {}

    std::function<double(const std::vector<double> &)>
    VariorumIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("VariorumIOGroup::agg_function(): unknown how to aggregate \"" +
                            signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_agg_function;
    }

    std::function<std::string(double)>
    VariorumIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("VariorumIOGroup::format_function(): unknown how to format \"" +
                            signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.m_format_function;
    }

    std::string VariorumIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("VariorumIOGroup::signal_description(): " + signal_name +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string result = "Invalid signal description: no description found.";
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result =  "    description: " + it->second.m_description + '\n'; // Includes alias_for if applicable
            result += "    units: " + IOGroup::units_to_string(it->second.m_units) + '\n';
            result += "    aggregation: " + Agg::function_to_name(it->second.m_agg_function) + '\n';
            result += "    domain: " + PlatformTopo::domain_type_to_name(GEOPM_DOMAIN_BOARD) + '\n';
            result += "    iogroup: VariorumIOGroup";
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("VariorumIOGroup::signal_description(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string VariorumIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception(
            "VariorumIOGroup::control_description(): there are no controls "
            "supported by the VariorumIOGroup",
            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int VariorumIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("VariorumIOGroup::signal_behavior(): " + signal_name +
                            "not valid for VariorumIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        int result = -1;
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result = it->second.m_behavior;
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("VariorumIOGroup::signal_description(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string VariorumIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::string VariorumIOGroup::plugin_name(void)
    {
        return "Variorum";
    }

    std::unique_ptr<IOGroup> VariorumIOGroup::make_plugin(void)
    {
        return geopm::make_unique<VariorumIOGroup>();
    }

    double VariorumIOGroup::read_time(const std::string &freshness_path) const
    {
        auto freshness = read_double_from_file(freshness_path, "");
        return (freshness - m_initial_freshness) / m_sample_rate;
    }

    void VariorumIOGroup::register_signal_alias(const std::string &alias_name,
                                           const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("VariorumIOGroup::register_signal_alias(): signal_name " + alias_name +
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

    void VariorumIOGroup::save_control(const std::string &save_path)
    {

    }

    void VariorumIOGroup::restore_control(const std::string &save_path)
    {

    }
}
