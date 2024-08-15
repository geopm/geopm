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

static std::map<std::pair<geopm_domain_e, int>, std::string> map_geopm_index_to_hwmon_path(const std::string &drm_directory, int num_tiles_per_gpu)
{
    std::map<std::pair<geopm_domain_e, int>, std::string> result;
    for (const auto &card_directory : geopm::list_directory_files(drm_directory)) {
        if (card_directory.find(GPU_CARD_PREFIX) != 0 || card_directory.length() <= GPU_CARD_PREFIX.length()) {
            continue;
        }
        auto card_index = std::stoi(card_directory.c_str() + GPU_CARD_PREFIX.length());

        std::ostringstream card_hwmon_oss;
        card_hwmon_oss << drm_directory << "/" << card_directory << "/device/hwmon";

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
                result.emplace(std::make_pair(GEOPM_DOMAIN_GPU, card_index), card_hwmon_oss.str() + "/" + hwmon_directory);
            }
            else if (geopm::string_begins_with(hwmon_name, HWMON_NAME_TILE_PREFIX)) {
                auto card_tile_index = std::stoi(hwmon_name.c_str() + HWMON_NAME_TILE_PREFIX.length());
                auto global_tile_index = card_index * num_tiles_per_gpu + card_tile_index;
                result.emplace(std::make_pair(GEOPM_DOMAIN_GPU_CHIP, global_tile_index), card_hwmon_oss.str() + "/" + hwmon_directory);
            }
        }
    }

    return result;
}

static std::map<int, std::string> load_resources_by_gpu_tile(const std::string &drm_directory, int num_tiles_per_gpu)
{
    std::map<int, std::string> result;
    for (const auto &card_directory : geopm::list_directory_files(drm_directory)) {
        if (card_directory.find(GPU_CARD_PREFIX) != 0 || card_directory.length() <= GPU_CARD_PREFIX.length()) {
            continue;
        }
        auto card_index = std::stoi(card_directory.c_str() + GPU_CARD_PREFIX.length());

        std::ostringstream card_gt_oss;
        card_gt_oss << drm_directory << "/" << card_directory << "/gt";
        for (const auto &tile_directory : geopm::list_directory_files(card_gt_oss.str())) {
            if (tile_directory.find(GPU_TILE_PREFIX) != 0 || tile_directory.length() <= GPU_TILE_PREFIX.length()) {
                continue;
            }
            auto card_tile_index = std::stoi(tile_directory.c_str() + GPU_TILE_PREFIX.length());
            if (card_tile_index < 0 || card_tile_index >= num_tiles_per_gpu) {
                throw geopm::Exception("DrmSysfsDriver encountered an unexpected GPU tile at " + card_gt_oss.str() + "/" + tile_directory,
                                       GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            auto global_tile_index = card_index * num_tiles_per_gpu + card_tile_index;
            result.emplace(global_tile_index, card_gt_oss.str() + "/" + tile_directory);
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

    DrmSysfsDriver::DrmSysfsDriver(const PlatformTopo &topo,
                                   const std::string &drm_directory,
                                   const std::string &driver_signal_prefix)
        : M_DRIVER_SIGNAL_PREFIX(driver_signal_prefix)
        , M_PROPERTIES{SysfsDriver::parse_properties_json(M_DRIVER_SIGNAL_PREFIX, drm_sysfs_json())}
        , M_DRM_HWMON_DIR_BY_GEOPM_DOMAIN(map_geopm_index_to_hwmon_path(
              drm_directory,
              (topo.num_domain(GEOPM_DOMAIN_GPU) > 0) ? (topo.num_domain(GEOPM_DOMAIN_GPU_CHIP) /
                                                         topo.num_domain(GEOPM_DOMAIN_GPU))
                                                      : 0))
        , M_DRM_RESOURCE_BY_GPU_TILE(load_resources_by_gpu_tile(
              drm_directory,
              (topo.num_domain(GEOPM_DOMAIN_GPU) > 0) ? (topo.num_domain(GEOPM_DOMAIN_GPU_CHIP) /
                                                         topo.num_domain(GEOPM_DOMAIN_GPU))
                                                      : 0))
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
            auto resource_it = M_DRM_RESOURCE_BY_GPU_TILE.find(domain_idx);
            if (resource_it == M_DRM_RESOURCE_BY_GPU_TILE.end()) {
                throw Exception("DrmSysfsDriver::attribute_path(): domain_idx " + std::to_string(domain_idx) + " does not have a drm entry.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            attribute_directory = resource_it->second;
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
        return M_DRIVER_SIGNAL_PREFIX;
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
        return std::make_unique<SysfsIOGroup>(std::make_shared<DrmSysfsDriver>(platform_topo(), DRM_DIRECTORY, plugin_name_drm()));
    }

    std::string DrmSysfsDriver::plugin_name_accel(void)
    {
        return "ACCEL";
    }

    std::unique_ptr<IOGroup> DrmSysfsDriver::make_plugin_accel(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<DrmSysfsDriver>(platform_topo(), ACCEL_DIRECTORY, plugin_name_accel()));
    }
}
