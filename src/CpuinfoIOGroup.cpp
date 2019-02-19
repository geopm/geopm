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

#include <cmath>
#include <cstring>

#include <fstream>
#include <algorithm>
#include <iterator>
#include "CpuinfoIOGroup.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "Agg.hpp"
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

    static double read_cpu_freq_sticker(const std::string &read_str)
    {
        double result = NAN;
        const std::string key = "model name";
        std::ifstream cpuinfo_file(read_str);
        if (!cpuinfo_file.good()) {
            throw Exception("Failed to open " + read_str + ": " + strerror(errno),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        while (std::isnan(result) && cpuinfo_file.good()) {
            std::string line;
            std::getline(cpuinfo_file, line);
            if (line.find(key) == 0 && line.find(':') != std::string::npos) {
                size_t colon_pos = line.find(':');
                bool match = true;
                for (size_t pos = key.size(); pos != colon_pos; ++pos) {
                    if (!std::isspace(line[pos])) {
                        match = false;
                    }
                }
                if (!match) {
                    continue;
                }
                std::transform(line.begin(), line.end(), line.begin(),
                               [](unsigned char c) {
                                   return std::tolower(c);
                               });
                std::string unit_str[3] = {"ghz", "mhz", "khz"};
                double unit_factor[3] = {1e9, 1e6, 1e3};
                for (int unit_idx = 0; unit_idx != 3; ++unit_idx) {
                    size_t unit_pos = line.find(unit_str[unit_idx]);
                    if (unit_pos != std::string::npos) {
                        std::string value_str = line.substr(0, unit_pos);
                        while (std::isspace(value_str.back())) {
                            value_str.erase(value_str.size() - 1);
                        }
                        size_t space_pos = value_str.rfind(' ');
                        if (space_pos != std::string::npos) {
                            value_str = value_str.substr(space_pos);
                        }
                        try {
                            result = unit_factor[unit_idx] * std::stod(value_str);
                        }
                        catch (const std::invalid_argument &ex) {
                            throw Exception("Invalid frequency: " + std::string(ex.what()),
                                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                        }
                    }
                }
            }
        }
        cpuinfo_file.close();
        return result;
    }

    CpuinfoIOGroup::CpuinfoIOGroup()
        :CpuinfoIOGroup("/proc/cpuinfo",
                        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq",
                        "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq")
    {

    }

    CpuinfoIOGroup::CpuinfoIOGroup(const std::string &cpu_info_path,
                                   const std::string &cpu_freq_min_path,
                                   const std::string &cpu_freq_max_path)
        : m_signal_value_map({{"CPUINFO::FREQ_MIN", read_cpu_freq(cpu_freq_min_path)},
                              {"CPUINFO::FREQ_STICKER", read_cpu_freq_sticker(cpu_info_path)},
                              {"CPUINFO::FREQ_MAX", read_cpu_freq(cpu_freq_max_path)},
                              {"CPUINFO::FREQ_STEP", 100e6}})
        , m_func_map({{"CPUINFO::FREQ_MIN", Agg::expect_same},
                      {"CPUINFO::FREQ_STICKER", Agg::expect_same},
                      {"CPUINFO::FREQ_MAX", Agg::expect_same},
                      {"CPUINFO::FREQ_STEP", Agg::expect_same}})
        , m_desc_map({{"CPUINFO::FREQ_MIN", "Minimum processor frequency in hertz"},
                      {"CPUINFO::FREQ_STICKER", "Processor base frequency in hertz"},
                      {"CPUINFO::FREQ_MAX", "Maximum processor frequency in hertz"},
                      {"CPUINFO::FREQ_STEP", "Step size between process frequency settings in hertz"}})
    {

    }

    std::set<std::string> CpuinfoIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_value_map) {
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
        return m_signal_value_map.find(signal_name) != m_signal_value_map.end();
    }

    bool CpuinfoIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int CpuinfoIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = PlatformTopo::M_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            if (std::isnan(m_signal_value_map.find(signal_name)->second)) {
                result = PlatformTopo::M_DOMAIN_INVALID;
            }
            else {
                result = PlatformTopo::M_DOMAIN_BOARD;
            }
        }
        return result;
    }

    int CpuinfoIOGroup::control_domain_type(const std::string &control_name) const
    {
        return PlatformTopo::M_DOMAIN_INVALID;
    }

    int CpuinfoIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("CpuinfoIOGroup::push_signal(): " + signal_name +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != PlatformTopo::M_DOMAIN_BOARD) {
            throw Exception("CpuinfoIOGroup::push_signal(): domain_type " + std::to_string(domain_type) +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return std::distance(m_signal_value_map.begin(), m_signal_value_map.find(signal_name));
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
        auto res_it = m_signal_value_map.begin();
        if (batch_idx >= 0 && batch_idx < (int) m_signal_value_map.size()) {
            std::advance(res_it, batch_idx);
            result = res_it->second;
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
        switch (domain_type) {
            case PlatformTopo::M_DOMAIN_BOARD:
            case PlatformTopo::M_DOMAIN_PACKAGE:
            case PlatformTopo::M_DOMAIN_CORE:
            case PlatformTopo::M_DOMAIN_CPU:
                break;
            case PlatformTopo::M_DOMAIN_INVALID:
            default:
                throw Exception("CpuinfoIOGroup::read_signal(): domain_type " + std::to_string(domain_type) +
                                "not valid for CpuinfoIOGroup",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        };
        return m_signal_value_map.find(signal_name)->second;
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
        auto it = m_func_map.find(signal_name);
        if (it == m_func_map.end()) {
            throw Exception("CpuinfoIOGroup::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    std::string CpuinfoIOGroup::signal_description(const std::string &signal_name) const
    {
        auto it = m_desc_map.find(signal_name);
        if (it == m_desc_map.end()) {
            throw Exception("CpuinfoIOGroup::signal_description(): " + signal_name +
                            "not valid for CpuinfoIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    std::string CpuinfoIOGroup::control_description(const std::string &control_name) const
    {
        return "";
    }

    std::string CpuinfoIOGroup::plugin_name(void)
    {
        return GEOPM_CPUINFO_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> CpuinfoIOGroup::make_plugin(void)
    {
        return std::unique_ptr<IOGroup>(new CpuinfoIOGroup);
    }
}
