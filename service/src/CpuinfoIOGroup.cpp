/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CpuinfoIOGroup.hpp"

#include <cmath>
#include <cstring>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "Cpuid.hpp"
#include "config.h"

#define GEOPM_CPUINFO_IO_GROUP_PLUGIN_NAME "CPUINFO"

namespace geopm
{
    static double read_cpu_freq(const std::string &read_str)
    {
        double result = NAN;
        std::ifstream ifs(read_str);
        if (ifs.is_open()) {
            std::string line;
            getline(ifs, line);
            ifs.close();
            try {
                result = 1e3 * std::stod(line);
            }
            catch (const std::invalid_argument &ex) {
                throw Exception("Invalid frequency: " + std::string(ex.what()),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        else {
            throw Exception("Failed to open " + read_str + ": " + strerror(errno),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    static double read_cpuid_freq_sticker(void)
    {
        return Cpuid::make_unique()->freq_sticker();
    }

    static double read_cpu_freq_sticker(void)
    {
        double result = read_cpuid_freq_sticker();
        if (result == 0) {
            throw Exception("Sticker frequency not supported by CPUID",
                            GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }
        return result;
    }

    CpuinfoIOGroup::CpuinfoIOGroup()
        :CpuinfoIOGroup("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq",
                        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq",
                        read_cpu_freq_sticker())
    {

    }

    CpuinfoIOGroup::CpuinfoIOGroup(const std::string &cpu_freq_min_path,
                                   const std::string &cpu_freq_max_path,
                                   double cpu_freq_sticker)
        : m_signal_available({{"CPUINFO::FREQ_MIN", {
                                   read_cpu_freq(cpu_freq_min_path),
                                   M_UNITS_HERTZ,
                                   Agg::expect_same,
                                   "Minimum processor frequency"}},
                              {"CPUINFO::FREQ_STICKER", {
                                   cpu_freq_sticker,
                                   M_UNITS_HERTZ,
                                   Agg::expect_same,
                                   "Processor base frequency"}},
                              {"CPUINFO::FREQ_MAX", {
                                   read_cpu_freq(cpu_freq_max_path),
                                   M_UNITS_HERTZ,
                                   Agg::expect_same,
                                   "Maximum processor frequency"}},
                              {"CPUINFO::FREQ_STEP", {
                                   100e6,
                                   M_UNITS_HERTZ,
                                   Agg::expect_same,
                                   "Step size between processor frequency settings"}}})
    {
        if (read_signal("CPUINFO::FREQ_MAX", GEOPM_DOMAIN_BOARD, 0) <=
            read_signal("CPUINFO::FREQ_MIN", GEOPM_DOMAIN_BOARD, 0)) {
            throw Exception("CpuinfoIOGroup::CpuinfoIOGroup(): Max frequency less than min",
                            GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }
        else if (read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0) <
                 read_signal("CPUINFO::FREQ_MIN", GEOPM_DOMAIN_BOARD, 0)) {
            throw Exception("CpuinfoIOGroup::CpuinfoIOGroup(): Sticker frequency less than min",
                            GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }
        else if (read_signal("CPUINFO::FREQ_STICKER", GEOPM_DOMAIN_BOARD, 0) >
                 read_signal("CPUINFO::FREQ_MAX", GEOPM_DOMAIN_BOARD, 0)) {
            throw Exception("CpuinfoIOGroup::CpuinfoIOGroup(): Sticker frequency greater than max",
                            GEOPM_ERROR_PLATFORM_UNSUPPORTED, __FILE__, __LINE__);
        }

        register_signal_alias("CPU_FREQUENCY_MIN_AVAIL", "CPUINFO::FREQ_MIN");
        register_signal_alias("CPU_FREQUENCY_STICKER", "CPUINFO::FREQ_STICKER");
        register_signal_alias("CPU_FREQUENCY_STEP", "CPUINFO::FREQ_STEP");
    }

    void CpuinfoIOGroup::register_signal_alias(const std::string &alias_name,
                                               const std::string &signal_name)
    {
        if (m_signal_available.find(alias_name) != m_signal_available.end()) {
            throw Exception("CpuinfoIOGroup::register_signal_alias(): signal_name " + alias_name +
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
        m_signal_available[alias_name].description =
            m_signal_available[signal_name].description + '\n' + "    alias_for: " + signal_name;
    }

    std::set<std::string> CpuinfoIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    std::set<std::string> CpuinfoIOGroup::control_names(void) const
    {
        return {};
    }

    bool CpuinfoIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    bool CpuinfoIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int CpuinfoIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            if (std::isnan(m_signal_available.find(signal_name)->second.value)) {
                throw Exception("CpuinfoIOGroup::signal_domain_type(): signal name " + signal_name +
                                " is valid but the value read is NaN.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            else {
                result = GEOPM_DOMAIN_BOARD;
            }
        }
        return result;
    }

    int CpuinfoIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int CpuinfoIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CpuinfoIOGroup::push_signal(): " + signal_name +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("CpuinfoIOGroup::push_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return std::distance(m_signal_available.begin(), m_signal_available.find(signal_name));
    }

    int CpuinfoIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("CpuinfoIOGroup::push_control(): there are no controls supported by the CpuinfoIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void CpuinfoIOGroup::read_batch(void)
    {
    }

    void CpuinfoIOGroup::write_batch(void)
    {
    }

    double CpuinfoIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        auto res_it = m_signal_available.begin();
        if (batch_idx >= 0 && batch_idx < (int) m_signal_available.size()) {
            std::advance(res_it, batch_idx);
            result = res_it->second.value;
        }
        else {
            throw Exception("CpuinfoIOGroup::sample(): batch_idx " + std::to_string(batch_idx) +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return result;
    }

    void CpuinfoIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("CpuinfoIOGroup::adjust(): there are no controls supported by the CpuinfoIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double CpuinfoIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CpuinfoIOGroup::read_signal(): " + signal_name +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != GEOPM_DOMAIN_BOARD) {
            throw Exception("CpuinfoIOGroup:read_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_available.find(signal_name)->second.value;
    }

    void CpuinfoIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("CpuinfoIOGroup::write_control(): there are no controls supported by the CpuinfoIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void CpuinfoIOGroup::save_control(void)
    {

    }

    void CpuinfoIOGroup::restore_control(void)
    {

    }

    std::function<double(const std::vector<double> &)> CpuinfoIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("CpuinfoIOGroup::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.agg_function;
    }

    std::function<std::string(double)> CpuinfoIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("CpuinfoIOGroup::format_function(): unknown how to format \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return string_format_double;
    }

    std::string CpuinfoIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CpuinfoIOGroup::signal_description(): signal_name " + signal_name +
                            " not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string result = "Invalid signal description: no description found.";
        auto it = m_signal_available.find(signal_name);
        if (it != m_signal_available.end()) {
            result =  "    description: " + it->second.description + '\n'; // Includes alias_for if applicable
            result += "    units: " + IOGroup::units_to_string(it->second.units) + '\n';
            result += "    aggregation: " + Agg::function_to_name(it->second.agg_function) + '\n';
            result += "    domain: " + platform_topo().domain_type_to_name(GEOPM_DOMAIN_BOARD) + '\n';
            result += "    iogroup: CpuinfoIOGroup";
        }
#ifdef GEOPM_DEBUG
        else {
            throw Exception("CpuinfoIOGroup::signal_description(): signal valid but not found in map",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
        return result;
    }

    std::string CpuinfoIOGroup::control_description(const std::string &control_name) const
    {
        return "";
    }

    int CpuinfoIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CpuinfoIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT;
    }

    std::string CpuinfoIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::string CpuinfoIOGroup::plugin_name(void)
    {
        return GEOPM_CPUINFO_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> CpuinfoIOGroup::make_plugin(void)
    {
        return std::unique_ptr<IOGroup>(new CpuinfoIOGroup);
    }

    void CpuinfoIOGroup::save_control(const std::string &save_path)
    {

    }

    void CpuinfoIOGroup::restore_control(const std::string &save_path)
    {

    }
}
