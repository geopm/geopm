/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "PowercapSysfsDriver.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>

#include "geopm/Helper.hpp"

#include "SysfsIOGroup.hpp"
#include "RolloverGenerator.hpp"
#include "geopm/PlatformTopo.hpp"

static const std::string POWERCAP_DIRECTORY = "/sys/class/powercap";
static const std::string POWERCAP_CPU_PREFIX = "POWERCAP::CPU_";
static const std::string POWERCAP_DRAM_PREFIX = "POWERCAP::DRAM_";

static std::map<std::string, std::string> load_powercap_resource_by_name(const std::string &powercap_directory)
{
    std::map<std::string, std::string> result;

    for (const auto &policy_file : geopm::list_directory_files(powercap_directory)) {
        if (policy_file.find("intel-rapl:") != 0) {
            continue;
        }

        std::ostringstream oss;
        oss << powercap_directory << "/" << policy_file << "/name";
        auto name_path = oss.str();

        int name_fd = open(name_path.c_str(), O_RDONLY);
        if (name_fd == -1) {
            throw geopm::Exception("PowercapSysfsDriver failed to open " + name_path,
                                   errno, __FILE__, __LINE__);
        }
        char name_buf[geopm::SysfsDriver::M_IO_BUFFER_SIZE] = {};
        int read_bytes = pread(name_fd, name_buf, sizeof(name_buf) - 1, 0);
        close(name_fd);
        if (read_bytes < 0) {
            throw geopm::Exception("PowercapSysfsDriver failed to read " + name_path,
                                   errno, __FILE__, __LINE__);
        }
        if (read_bytes == sizeof(name_buf) - 1) {
            throw geopm::Exception("PowercapSysfsDriver truncated read from " + name_path,
                                   errno, __FILE__, __LINE__);
        }

        std::string name(name_buf);
        if (!name.empty() && name.back() == '\n') {
            name.pop_back();
        }
        result.emplace(name, policy_file);
    }

    return result;
}

static std::string name_to_resource_key(const std::string &name, int domain_idx)
{
    std::string resource_key;
    if (name.find(POWERCAP_CPU_PREFIX) == 0) {
        resource_key = "package-" + std::to_string(domain_idx);
    }
    else if (name.find(POWERCAP_DRAM_PREFIX) == 0) {
        resource_key = "dram";
    }
    else {
        throw geopm::Exception("PowercapSysfsDriver: unknown name: " + name,
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    return resource_key;
}

static std::string name_to_property_key(const std::string &name)
{
    std::string property_key;
    auto substr_pos = name.find(POWERCAP_CPU_PREFIX);
    if (substr_pos == std::string::npos) {
        substr_pos = name.find(POWERCAP_DRAM_PREFIX);
    }
    if (substr_pos == std::string::npos) {
        throw geopm::Exception("PowercapSysfsDriver::attribute_path(): No such domain prefix "
                               + name,
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
    }
    return name.substr(substr_pos);
}

namespace geopm
{
    const std::string powercap_sysfs_json(void);

    PowercapSysfsDriver::PowercapSysfsDriver()
        : PowercapSysfsDriver(POWERCAP_DIRECTORY)
    {
    }

    PowercapSysfsDriver::PowercapSysfsDriver(const std::string &powercap_directory)
        : M_PROPERTIES{SysfsDriver::parse_properties_json(plugin_name(), powercap_sysfs_json())}
        , M_POWERCAP_RESOURCE_BY_NAME(load_powercap_resource_by_name(powercap_directory))
        , M_POWERCAP_DIRECTORY(powercap_directory)
        , m_rollover_factor(0.0)
    {
        try {
            std::string factor_path = attribute_path("POWERCAP::CPU_MAX_ENERGY_RANGE", 0);
            std::string contents = geopm::read_file(factor_path);
            m_rollover_factor = 1e-6 * std::stoll(contents);
         }
         catch (...) {
             throw geopm::Exception("PowercapSysfsDriver: Unable to parse RAPL rollover from sysfs",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
         }
    }

    int PowercapSysfsDriver::domain_type(const std::string &name) const
    {
        if (M_PROPERTIES.find(name) == M_PROPERTIES.end()) {
            throw Exception("PowercapSysfsDriver: unknown name: " + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return GEOPM_DOMAIN_PACKAGE;
    }

    std::string PowercapSysfsDriver::attribute_path(const std::string &name, int domain_idx)
    {
        std::string resource_key = name_to_resource_key(name, domain_idx);
        std::string property_key = name_to_property_key(name);
        auto property_it = M_PROPERTIES.find(property_key);
        if (property_it == M_PROPERTIES.end()) {
            throw Exception("PowercapSysfsDriver::attribute_path(): No such signal "
                            + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        auto resource_it = M_POWERCAP_RESOURCE_BY_NAME.find(resource_key);
        if (resource_it == M_POWERCAP_RESOURCE_BY_NAME.end()) {
            throw Exception("PowercapSysfsDriver::attribute_path(): No such resource "
                            + resource_key,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::ostringstream oss;
        oss << M_POWERCAP_DIRECTORY << "/" << resource_it->second
            << "/" << property_it->second.attribute;
        return oss.str();
    }

    std::function<double(const std::string &)> PowercapSysfsDriver::signal_parse(const std::string &signal_name) const
    {
        std::string resource_key = name_to_resource_key(signal_name, 0);
        std::string property_key = name_to_property_key(signal_name);
        auto prop_it = M_PROPERTIES.find(property_key);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("PowercapSysfsDriver::signal_parse(): Unknown signal name: " + signal_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
        std::shared_ptr<RolloverGenerator> rollover_ptr = nullptr;
        if (signal_name.find("ENERGY") != std::string::npos) {
            rollover_ptr = std::make_shared<RolloverGenerator>();
            rollover_ptr->set_factor(m_rollover_factor);
        }
        return [scaling_factor, rollover_ptr = std::move(rollover_ptr)](const std::string &content) {
            double result = static_cast<double>(NAN);
            try {
                result = static_cast<double>(std::stoull(content) * scaling_factor);
                if (rollover_ptr != nullptr) {
                    result = rollover_ptr->update(result);
                }
            }
            catch (const std::invalid_argument &ex) {}
            catch (const std::out_of_range &ex) {}
            return result;
        };
    }

    std::function<std::string(double)> PowercapSysfsDriver::control_gen(const std::string &control_name) const
    {
        std::string resource_key = name_to_resource_key(control_name, 0);
        std::string property_key = name_to_property_key(control_name);
        auto prop_it = M_PROPERTIES.find(property_key);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("PowercapSysfsDriver::control_gen(): Unknown control name: " + control_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
        return [scaling_factor](double value) {
            return std::to_string(std::llround(value / scaling_factor));
        };
    }

    std::string PowercapSysfsDriver::driver(void) const
    {
        return plugin_name();
    }

    std::map<std::string, SysfsDriver::properties_s> PowercapSysfsDriver::properties(void) const
    {
        return M_PROPERTIES;
    }

    std::string PowercapSysfsDriver::plugin_name(void)
    {
        return "POWERCAP";
    }

    std::unique_ptr<IOGroup> PowercapSysfsDriver::make_plugin(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<PowercapSysfsDriver>());
    }
}
