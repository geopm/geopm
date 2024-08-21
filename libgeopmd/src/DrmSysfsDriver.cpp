/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DrmSysfsDriver.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cmath>
#include <cstring>
#include <sstream>

#include "DrmGpuTopo.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_topo.h"

#include "SysfsIOGroup.hpp"

static const std::string DRM_DIRECTORY = "/sys/class/drm";
static const std::string ACCEL_DIRECTORY = "/sys/class/accel";
static const std::string GPU_CARD_PREFIX = "card";
static const std::string GPU_TILE_PREFIX = "gt";
static const std::string HWMON_PREFIX = "hwmon";
static const std::string HWMON_NAME_CARD = "i915\n";
static const std::string HWMON_NAME_TILE_PREFIX = "i915_gt";
static const std::string GPU_SIGNAL_NAME_SUFFIX = "::GPU";
static const std::string TILE_SIGNAL_NAME_SUFFIX = "::GPU_CHIP";

struct hwmon_paths_s {
    std::vector<std::string> m_card_paths; /* Expect size 0..1 */
    std::map<int /* tile index within card */, std::string> m_gt_paths;
};

// Return card/tile hwmon paths that relate to a given drm card path
static hwmon_paths_s card_path_to_hwmon_paths(const std::string &card_path)
{
    hwmon_paths_s result = {{}, {}};
    std::ostringstream card_hwmon_oss;
    card_hwmon_oss << card_path << "/device/hwmon";

    std::vector<std::string> hwmon_files;
    try {
        hwmon_files = geopm::list_directory_files(card_hwmon_oss.str());
    }
    catch (const geopm::Exception &ex) {
        // If this drm device doesn't have any linked hwmon attributes,
        // simply don't attempt to map hwmon.
    }

    for (const auto &hwmon_directory : hwmon_files) {
        if (hwmon_directory.find(HWMON_PREFIX) != 0 || hwmon_directory.length() <= HWMON_PREFIX.length()) {
            continue;
        }
        auto hwmon_index = std::stoi(hwmon_directory.c_str() + HWMON_PREFIX.length());
        if (hwmon_index < 0) {
            throw geopm::Exception("DrmSysfsDriver encountered an unexpected hwmon directory at " + card_hwmon_oss.str() + "/" + hwmon_directory,
                                   GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        std::string hwmon_name = geopm::read_file(card_hwmon_oss.str() + "/" + hwmon_directory + "/name");
        if (hwmon_name == HWMON_NAME_CARD) {
            result.m_card_paths.push_back(card_hwmon_oss.str() + "/" + hwmon_directory);
        }
        else if (geopm::string_begins_with(hwmon_name, HWMON_NAME_TILE_PREFIX)) {
            auto card_tile_index = std::stoi(hwmon_name.c_str() + HWMON_NAME_TILE_PREFIX.length());
            result.m_gt_paths.emplace(card_tile_index, card_hwmon_oss.str() + "/" + hwmon_directory);
        }
    }

    return result;
}

static std::map<std::pair<geopm_domain_e, int>, std::string> map_geopm_index_to_hwmon_path(const geopm::DrmGpuTopo &gpu_topo)
{
    std::map<std::pair<geopm_domain_e, int>, std::string> result;
    auto gts_per_card = gpu_topo.num_gpu(GEOPM_DOMAIN_GPU) == 0 ? 0
        : gpu_topo.num_gpu(GEOPM_DOMAIN_GPU_CHIP) / gpu_topo.num_gpu(GEOPM_DOMAIN_GPU);
    for (int gpu_idx = 0; gpu_idx < gpu_topo.num_gpu(); ++gpu_idx) {
        auto card_path = gpu_topo.card_path(gpu_idx);
        auto hwmon_paths = card_path_to_hwmon_paths(card_path);
        if (hwmon_paths.m_card_paths.size() > 1) {
            throw geopm::Exception("DrmSysfsDriver: multiple card-scoped hwmon objects found for " + card_path,
                                   GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        else if (hwmon_paths.m_card_paths.size() == 1) {
            result.emplace(std::make_pair(GEOPM_DOMAIN_GPU, gpu_idx), hwmon_paths.m_card_paths[0]);
        }

        if (static_cast<int>(hwmon_paths.m_gt_paths.size()) > gts_per_card) {
            throw geopm::Exception("DrmSysfsDriver: multiple tile-scoped hwmon objects per GPU tile found for " + card_path,
                                   GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        else {
            for (const auto &it : hwmon_paths.m_gt_paths) {
                result.emplace(std::make_pair(GEOPM_DOMAIN_GPU_CHIP, gts_per_card * gpu_idx + it.first), it.second);
            }
        }
    }
    return result;
}

static bool signal_name_is_from_hwmon(const std::string &signal_name, const std::string &driver_signal_prefix)
{
    return geopm::string_begins_with(signal_name, driver_signal_prefix + "::HWMON::");
}

namespace geopm
{
    const std::string drm_sysfs_json(void);

    DrmSysfsDriver::DrmSysfsDriver(const std::string &drm_directory,
                                   const std::string &driver_signal_prefix)
        : m_drm_topo(drm_directory)
        , M_DRIVER_SIGNAL_PREFIX(driver_signal_prefix)
        , M_PROPERTIES{SysfsDriver::parse_properties_json(M_DRIVER_SIGNAL_PREFIX, drm_sysfs_json())}
        , M_DRM_HWMON_DIR_BY_GEOPM_DOMAIN(map_geopm_index_to_hwmon_path(m_drm_topo))
    {
    }

    int DrmSysfsDriver::domain_type(const std::string &name) const
    {
        // So far, all of the supported i915 DRM signals are tile-scoped and
        // most of the i915 hwmon signals are card-scoped.
        if (signal_name_is_from_hwmon(name, M_DRIVER_SIGNAL_PREFIX) && !geopm::string_ends_with(name, TILE_SIGNAL_NAME_SUFFIX)) {
            return GEOPM_DOMAIN_GPU;
        }
        else {
            return GEOPM_DOMAIN_GPU_CHIP;
        }
    }

    std::string DrmSysfsDriver::attribute_path(const std::string &name,
                                               int domain_idx)
    {
        std::string attribute_directory;
        auto signal_domain_type = static_cast<geopm_domain_e>(domain_type(name));

        if (signal_name_is_from_hwmon(name, M_DRIVER_SIGNAL_PREFIX)) {
            auto resource_it = M_DRM_HWMON_DIR_BY_GEOPM_DOMAIN.find(std::make_pair(signal_domain_type, domain_idx));
            if (resource_it == M_DRM_HWMON_DIR_BY_GEOPM_DOMAIN.end()) {
                throw Exception("DrmSysfsDriver::attribute_path(): domain " + std::to_string(signal_domain_type) + " domain_idx " + std::to_string(domain_idx) + " does not have a hwinfo entry.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            attribute_directory = resource_it->second;
        }
        else {
            attribute_directory = m_drm_topo.gt_path(domain_idx);
        }

        auto property_it = M_PROPERTIES.find(name);
        if (property_it == M_PROPERTIES.end()) {
            throw Exception("DrmSysfsDriver::attribute_path(): No such signal " + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        std::ostringstream oss;
        oss << attribute_directory
            << "/" << property_it->second.attribute;
        return oss.str();
    }

    std::function<double(const std::string &)> DrmSysfsDriver::signal_parse(const std::string &signal_name) const
    {
        auto prop_it = M_PROPERTIES.find(signal_name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("DrmSysfsDriver::signal_parse(): Unknown signal name: " + signal_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
        return [scaling_factor](const std::string &content) {
            double result = static_cast<double>(NAN);
            try {
                result = static_cast<double>(std::stol(content) * scaling_factor);
            }
            catch (const std::invalid_argument &ex) {
            }
            catch (const std::out_of_range &ex) {
            }
            return result;
        };
    }

    std::function<std::string(double)> DrmSysfsDriver::control_gen(const std::string &control_name) const
    {
        auto prop_it = M_PROPERTIES.find(control_name);
        if (prop_it == M_PROPERTIES.end()) {
            throw Exception("DrmSysfsDriver::control_gen(): Unknown control name: " + control_name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        double scaling_factor = prop_it->second.scaling_factor;
        return [scaling_factor](double value) {
            return std::to_string(std::llround(value / scaling_factor));
        };
    }

    std::string DrmSysfsDriver::driver(void) const
    {
        return M_DRIVER_SIGNAL_PREFIX + " from driver: " + m_drm_topo.driver_name();
    }

    std::map<std::string, SysfsDriver::properties_s> DrmSysfsDriver::properties(void) const
    {
        return M_PROPERTIES;
    }

    std::string DrmSysfsDriver::plugin_name_drm(void)
    {
        return "DRM";
    }

    std::unique_ptr<IOGroup> DrmSysfsDriver::make_plugin_drm(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<DrmSysfsDriver>(DRM_DIRECTORY, plugin_name_drm()));
    }

    std::string DrmSysfsDriver::plugin_name_accel(void)
    {
        return "ACCEL";
    }

    std::unique_ptr<IOGroup> DrmSysfsDriver::make_plugin_accel(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<DrmSysfsDriver>(ACCEL_DIRECTORY, plugin_name_accel()));
    }
}
