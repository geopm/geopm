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
        : m_properties {SysfsDriver::parse_properties_json("{}")}
    {

    }

    int CpufreqSysfsDriver::domain_type(const std::string &name) const
    {
        return 0;
    }

    std::string CpufreqSysfsDriver::signal_path(const std::string &signal_name,
                                                int domain_idx)
    {
        std::string result;
        return result;
    }

    std::string CpufreqSysfsDriver::control_path(const std::string &control_name,
                                                 int domain_idx) const
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

    std::map<std::string, SysfsDriver::properties_s> CpufreqSysfsDriver::properties(void) const
    {
        std::map<std::string, SysfsDriver::properties_s> result;
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
