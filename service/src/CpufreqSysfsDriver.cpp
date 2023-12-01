/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "CpufreqSysfsDriver.hpp"
#include "SysfsIOGroup.hpp"

namespace geopm
{
    CpufreqSysfsDriver::CpufreqSysfsDriver()
        : m_properties {SysfsDriver::parse_properties_json(properties_json())}
    {

    }

    std::vector<std::string> CpufreqSysfsDriver::signal_names(void) const
    {
        std::vector<std::string> result;
        return result;
    }

    std::vector<std::string> CpufreqSysfsDriver::control_names(void) const
    {
        std::vector<std::string> result;
        return result;
    }

    int CpufreqSysfsDriver::domain_type(const std::string &name) const
    {
        return 0;
    }

    std::string CpufreqSysfsDriver::signal_path(const std::string &signal_name,
                                                int domain_type,
                                                int domain_idx)
    {
        std::string result;
        return result;
    }

    std::string CpufreqSysfsDriver::control_path(const std::string &control_name,
                                                 int domain_type,
                                                 int domain_idx) const
    {
        std::string result;
        return result;
    }

    double CpufreqSysfsDriver::signal_parse(const std::string &signal_name,
                                            const std::string &content) const
    {
        double result = 0.0;
        return result;
    }

    std::string CpufreqSysfsDriver::control_gen(const std::string &control_name,
                                                double setting) const
    {
        std::string result;
        return result;
    }

    std::function<double(const std::string&)> CpufreqSysfsDriver::signal_parse(const std::string &signal_name) const
    {
        return [] (const std::string &content)
        {
            return std::stod(content);
        };
    }

    std::function<std::string(double)> CpufreqSysfsDriver::control_gen(const std::string &control_name) const
    {
        return [] (double value)
        {
            return std::to_string(value);
        };
    }

    std::string CpufreqSysfsDriver::driver(void) const
    {
        std::string result;
        return result;
    }

    struct SysfsDriver::properties_s CpufreqSysfsDriver::properties(const std::string &name) const
    {
        struct SysfsDriver::properties_s result {};
        return result;
    }

    std::string CpufreqSysfsDriver::properties_json(void) const
    {
        std::string result;
        return result;
    }

    std::string CpufreqSysfsDriver::plugin_name(void)
    {
        return "CPUFREQ";
    }

    std::unique_ptr<IOGroup> CpufreqSysfsDriver::make_plugin(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<CpufreqSysfsDriver>());
    }
}
