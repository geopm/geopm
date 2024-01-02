/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "CpufreqSysfsDriver.hpp"

#include <fcntl.h>
#include <unistd.h>

#include "geopm/Helper.hpp"
#include "SysfsIOGroup.hpp"

#include <cmath>
#include <cstring>
#include <sstream>

// Arbitrary buffer size. We're generally looking at integer values much shorter
// than 100 digits in length. The IOGroup performs string truncation checks in
// case that ever changes.
static const std::string CPUFREQ_DIRECTORY = "/sys/devices/system/cpu/cpufreq";

static std::map<int, std::string> load_cpufreq_resources_by_cpu(const std::string &cpufreq_directory)
{
    char cpu_buf[geopm::SysfsDriver::M_IO_BUFFER_SIZE] = {0};
    std::map<int, std::string> result;
    for (const auto &policy_file : geopm::list_directory_files(cpufreq_directory)) {
        if (policy_file.find("policy") != 0) {
            continue;
        }

        std::ostringstream oss;
        oss << cpufreq_directory << "/" << policy_file;
        auto resource_path = oss.str();
        oss << "/affected_cpus";
        auto cpu_map_path = oss.str();

        int cpu_map_fd = open(cpu_map_path.c_str(), O_RDONLY);
        if (cpu_map_fd == -1) {
            throw geopm::Exception("SysfsIOGroup failed to open " + cpu_map_path,
                                   errno, __FILE__, __LINE__);
        }
        int read_bytes = pread(cpu_map_fd, cpu_buf, sizeof cpu_buf - 1, 0);
        close(cpu_map_fd);
        if (read_bytes < 0) {
            throw geopm::Exception("SysfsIOGroup failed to read " + cpu_map_path,
                                   errno, __FILE__, __LINE__);
        }
        if (read_bytes == sizeof cpu_buf - 1) {
            throw geopm::Exception("SysfsIOGroup truncated read from " + cpu_map_path,
                                   errno, __FILE__, __LINE__);
        }
        result.emplace(std::stoi(cpu_buf), resource_path);
    }

    return result;
}

namespace geopm
{
    const std::string cpufreq_sysfs_json(void);

    CpufreqSysfsDriver::CpufreqSysfsDriver()
        : CpufreqSysfsDriver(CPUFREQ_DIRECTORY)
    {
    }

    CpufreqSysfsDriver::CpufreqSysfsDriver(const std::string &cpufreq_directory)
        : M_PROPERTIES{SysfsDriver::parse_properties_json(plugin_name(), cpufreq_sysfs_json())}
        , M_CPUFREQ_RESOURCE_BY_CPU(load_cpufreq_resources_by_cpu(cpufreq_directory))
    {

    }

    int CpufreqSysfsDriver::domain_type(const std::string &) const
    {
        return GEOPM_DOMAIN_CPU;
    }

    std::string CpufreqSysfsDriver::attribute_path(const std::string &name,
                                                   int domain_idx)
    {
        auto resource_it = M_CPUFREQ_RESOURCE_BY_CPU.find(domain_idx);
        if (resource_it == M_CPUFREQ_RESOURCE_BY_CPU.end()) {
            throw Exception("CpufreqSysfsDriver::attribute_path(): domain_idx "
                            + std::to_string(domain_idx)
                            + " does not have a cpufreq entry.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        auto property_it = M_PROPERTIES.find(name);
        if (property_it == M_PROPERTIES.end()) {
            throw Exception("CpufreqSysfsDriver::attribute_path(): No such signal "
                            + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        
        std::ostringstream oss;
        oss << resource_it->second
            << "/" << property_it->second.attribute;
        return oss.str();
    }

    std::function<double(const std::string &)> CpufreqSysfsDriver::signal_parse(const std::string &signal_name) const
    {
        double scaling_factor = M_PROPERTIES.at(signal_name).scaling_factor;
        return [scaling_factor](const std::string &content) {
            if (content.find("<unsupported>") != content.npos) {
                return static_cast<double>(NAN);
            }
            else {
                return static_cast<double>(std::stoi(content) * scaling_factor);
            }
        };
    }

    std::function<std::string(double)> CpufreqSysfsDriver::control_gen(const std::string &control_name) const
    {
        double scaling_factor = M_PROPERTIES.at(control_name).scaling_factor;
        return [scaling_factor](double value) {
            return std::to_string(std::llround(value / scaling_factor));
        };
    }

    std::string CpufreqSysfsDriver::driver(void) const
    {
        return "cpufreq";
    }

    std::map<std::string, SysfsDriver::properties_s> CpufreqSysfsDriver::properties(void) const
    {
        return M_PROPERTIES;
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
