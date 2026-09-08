/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "UncoreFreqSysfsDriver.hpp"

#include <cmath>
#include <cstdio>
#include <sstream>
#include <string>
#include <exception>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_topo.h"

#include "SysfsIOGroup.hpp"

static const std::string UNCORE_DIRECTORY = "/sys/devices/system/cpu/intel_uncore_frequency";
// The only attribute served from the (root-only) uncore* domain
// directories rather than the package_*_die_* fan-out directory.
static const std::string UNCORE_STATUS_ATTRIBUTE = "current_freq_khz";

// Read a small integer identity file (e.g. package_id, domain_id).
// Returns false if the file cannot be read or parsed; these files are
// root-only (0400) so this fails gracefully in the non-privileged path.
static bool try_read_int(const std::string &path, int &value)
{
    std::string content;
    try {
        content = geopm::read_file(path);
    }
    catch (const std::exception &ex) {
        return false;
    }
    try {
        value = std::stoi(content);
    }
    catch (const std::exception &ex) {
        return false;
    }
    return true;
}

// Map package index -> package_*_die_* directory path.  These are the
// world-readable fan-out directories used for max/min controls.  The
// package index is parsed from the directory name, so no privileged
// access is required.  If a package has multiple die directories the
// first one encountered is used (Phase 1 is package-scoped).
static std::map<int, std::string> load_control_dirs(const std::string &uncore_directory)
{
    std::map<int, std::string> result;
    for (const auto &entry : geopm::list_directory_files(uncore_directory)) {
        if (entry.rfind("package_", 0) != 0) {
            continue;
        }
        int package_idx = -1;
        int die_idx = -1;
        if (std::sscanf(entry.c_str(), "package_%d_die_%d", &package_idx, &die_idx) != 2) {
            continue;
        }
        result.emplace(package_idx, uncore_directory + "/" + entry);
    }
    return result;
}

// Map package index -> representative uncore* directory path.  The
// uncore* directories expose current_freq_khz along with the root-only
// identity files package_id and domain_id.  For each package the domain
// with the lowest domain_id is chosen as the representative.  Entries
// whose identity files cannot be read (non-privileged path) are skipped.
static std::map<int, std::string> load_status_dirs(const std::string &uncore_directory)
{
    std::map<int, std::string> result;
    std::map<int, int> best_domain_by_package;
    for (const auto &entry : geopm::list_directory_files(uncore_directory)) {
        if (entry.rfind("uncore", 0) != 0) {
            continue;
        }
        std::string path = uncore_directory + "/" + entry;
        int package_idx = -1;
        int domain_idx = -1;
        if (!try_read_int(path + "/package_id", package_idx) ||
            !try_read_int(path + "/domain_id", domain_idx)) {
            continue;
        }
        auto best_it = best_domain_by_package.find(package_idx);
        if (best_it == best_domain_by_package.end() || domain_idx < best_it->second) {
            best_domain_by_package[package_idx] = domain_idx;
            result[package_idx] = path;
        }
    }
    return result;
}

namespace geopm
{
    const std::string uncore_sysfs_json(void);

    UncoreFreqSysfsDriver::UncoreFreqSysfsDriver()
        : UncoreFreqSysfsDriver(UNCORE_DIRECTORY)
    {
    }

    UncoreFreqSysfsDriver::UncoreFreqSysfsDriver(const std::string &uncore_directory)
        : M_PROPERTIES{SysfsDriver::parse_properties_json(plugin_name(), uncore_sysfs_json())}
        , M_UNCORE_DIRECTORY(uncore_directory)
        , m_control_dir_by_package(load_control_dirs(uncore_directory))
        , m_status_dir_by_package(load_status_dirs(uncore_directory))
    {
    }

    int UncoreFreqSysfsDriver::domain_type(const std::string &name) const
    {
        if (M_PROPERTIES.find(name) == M_PROPERTIES.end()) {
            throw Exception("UncoreFreqSysfsDriver::domain_type(): Unknown signal name: " + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return GEOPM_DOMAIN_PACKAGE;
    }

    std::string UncoreFreqSysfsDriver::attribute_path(const std::string &name, int domain_idx)
    {
        auto prop_it = M_PROPERTIES.find(name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("UncoreFreqSysfsDriver::attribute_path(): No such signal: " + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        const std::string &attribute = prop_it->second.attribute;
        const std::map<int, std::string> &dir_by_package =
            (attribute == UNCORE_STATUS_ATTRIBUTE) ? m_status_dir_by_package
                                                   : m_control_dir_by_package;
        auto dir_it = dir_by_package.find(domain_idx);
        if (dir_it == dir_by_package.end()) {
            throw Exception("UncoreFreqSysfsDriver::attribute_path(): No sysfs directory for "
                            "domain index " + std::to_string(domain_idx) + " and signal: " + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return dir_it->second + "/" + attribute;
    }

    std::function<double(const std::string &)> UncoreFreqSysfsDriver::signal_parse(const std::string &signal_name) const
    {
        auto prop_it = M_PROPERTIES.find(signal_name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("UncoreFreqSysfsDriver::signal_parse(): Unknown signal name: " + signal_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
        return [scaling_factor](const std::string &content) {
            double result = static_cast<double>(NAN);
            try {
                result = static_cast<double>(std::stoull(content) * scaling_factor);
            }
            catch (const std::invalid_argument &ex) {}
            catch (const std::out_of_range &ex) {}
            return result;
        };
    }

    std::function<std::string(double)> UncoreFreqSysfsDriver::control_gen(const std::string &control_name) const
    {
        auto prop_it = M_PROPERTIES.find(control_name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("UncoreFreqSysfsDriver::control_gen(): Unknown control name: " + control_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
        return [scaling_factor](double value) {
            return std::to_string(std::llround(value / scaling_factor));
        };
    }

    std::string UncoreFreqSysfsDriver::driver(void) const
    {
        return plugin_name();
    }

    std::map<std::string, SysfsDriver::properties_s> UncoreFreqSysfsDriver::properties(void) const
    {
        return M_PROPERTIES;
    }

    std::string UncoreFreqSysfsDriver::plugin_name(void)
    {
        return "UNCORE";
    }

    std::unique_ptr<IOGroup> UncoreFreqSysfsDriver::make_plugin(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<UncoreFreqSysfsDriver>());
    }
}
