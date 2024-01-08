/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "CpufreqSysfsDriver.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <sstream>

#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_topo.h"

#include "SysfsIOGroup.hpp"

static const std::string CPUFREQ_DIRECTORY = "/sys/devices/system/cpu/cpufreq";

static std::map<std::string, std::vector<int>> load_cpufreq_cpus_by_resource(const std::string &cpufreq_directory)
{
    char cpu_buf[geopm::SysfsDriver::M_IO_BUFFER_SIZE] = {0};
    std::map<std::string, std::vector<int>> result;

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
            throw geopm::Exception("CpufreqSysfsDriver failed to open " + cpu_map_path,
                                   errno, __FILE__, __LINE__);
        }
        int read_bytes = pread(cpu_map_fd, cpu_buf, sizeof cpu_buf - 1, 0);
        close(cpu_map_fd);
        if (read_bytes < 0) {
            throw geopm::Exception("CpufreqSysfsDriver failed to read " + cpu_map_path,
                                   errno, __FILE__, __LINE__);
        }
        if (read_bytes == sizeof cpu_buf - 1) {
            throw geopm::Exception("CpufreqSysfsDriver truncated read from " + cpu_map_path,
                                   errno, __FILE__, __LINE__);
        }

        std::vector<int> affected_cpus;
        for (const auto &cpu_string : geopm::string_split(cpu_buf, " ")) {
            affected_cpus.push_back(std::stoi(cpu_string));
        }
        result.emplace(resource_path, affected_cpus);
    }

    return result;
}

// Given a map of (resource)->(vector of CPUs), produce a map of (cpu)->(resource)
static std::map<int, std::string> resources_by_cpu_from_cpus_by_resource(
    const std::map<std::string, std::vector<int> > &cpus_by_resource)
{
    std::map<int, std::string> resources_by_cpu;
    for (const auto &[resource, cpus] : cpus_by_resource) {
        for (auto cpu : cpus) {
            resources_by_cpu.emplace(cpu, resource);
        }
    }
    return resources_by_cpu;
}

namespace geopm
{
    const std::string cpufreq_sysfs_json(void);

    CpufreqSysfsDriver::CpufreqSysfsDriver()
        : CpufreqSysfsDriver(platform_topo(), CPUFREQ_DIRECTORY)
    {
    }

    CpufreqSysfsDriver::CpufreqSysfsDriver(const PlatformTopo &topo,
                                           const std::string &cpufreq_directory)
        : M_PROPERTIES{SysfsDriver::parse_properties_json(plugin_name(), cpufreq_sysfs_json())}
        , M_CPUFREQ_CPUS_BY_RESOURCE(load_cpufreq_cpus_by_resource(cpufreq_directory))
        , M_CPUFREQ_RESOURCE_BY_CPU(resources_by_cpu_from_cpus_by_resource(M_CPUFREQ_CPUS_BY_RESOURCE))
        , m_domain(GEOPM_DOMAIN_CPU)
        , m_topo(topo)
    {
        std::vector<geopm_domain_e> cpu_nested_domains = {
            GEOPM_DOMAIN_CPU, GEOPM_DOMAIN_CORE, GEOPM_DOMAIN_PACKAGE, GEOPM_DOMAIN_BOARD
        };
        for (auto outer_domain : cpu_nested_domains) {
            bool is_correct_domain = true;
            for (const auto &resource_cpus : M_CPUFREQ_CPUS_BY_RESOURCE) {
                auto affected_cpus = resource_cpus.second;
                std::set<int> affected_domain_indices;
                for (auto affected_cpu : affected_cpus) {
                    affected_domain_indices.insert(m_topo.domain_idx(outer_domain, affected_cpu));
                }
                if (affected_domain_indices.size() > 1) {
                    // The CPUs in this resource group span across multiple
                    // indices in the current topology domain, so their
                    // minimal common domain is at a more coarse level.
                    is_correct_domain = false;
                    break;
                }
            }
            if (is_correct_domain && m_topo.is_nested_domain(m_domain, outer_domain)) {
                // This cpufreq policy spans a domain that is more coarse
                // than the most-coarse scope observed so far.
                m_domain = outer_domain;
                break;
            }
        }
    }

    int CpufreqSysfsDriver::domain_type(const std::string &) const
    {
        return m_domain;
    }

    std::string CpufreqSysfsDriver::attribute_path(const std::string &name,
                                                   int domain_idx)
    {
        auto cpus_in_domain_idx = m_topo.domain_nested(GEOPM_DOMAIN_CPU, m_domain, domain_idx);

        auto resource_it = M_CPUFREQ_RESOURCE_BY_CPU.end();
        for (auto cpu_idx : cpus_in_domain_idx) {
            resource_it = M_CPUFREQ_RESOURCE_BY_CPU.find(cpu_idx);
            if (resource_it != M_CPUFREQ_RESOURCE_BY_CPU.end()) {
                // Multiple CPUs may map to this domain_idx, but we only
                // need to use the first mapping discovered.
                break;
            }
        }
        if (resource_it == M_CPUFREQ_RESOURCE_BY_CPU.end()) {
            throw Exception("CpufreqSysfsDriver::attribute_path(): domain_idx " + std::to_string(domain_idx) + " does not have a cpufreq entry.",
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
        auto prop_it = M_PROPERTIES.find(signal_name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("CpufreqSysfsDriver::signal_parse(): Unknown signal name: " + signal_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
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
        auto prop_it = M_PROPERTIES.find(control_name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("CpufreqSysfsDriver::control_gen(): Unknown control name: " + control_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
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
