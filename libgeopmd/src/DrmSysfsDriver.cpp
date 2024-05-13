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
static const std::string GPU_CARD_PREFIX = "card";
static const std::string GPU_TILE_PREFIX = "gt";

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

namespace geopm
{
    const std::string drm_sysfs_json(void);

    DrmSysfsDriver::DrmSysfsDriver()
        : DrmSysfsDriver(platform_topo(), DRM_DIRECTORY)
    {
    }

    DrmSysfsDriver::DrmSysfsDriver(const PlatformTopo &topo,
                                   const std::string &drm_directory)
        : M_PROPERTIES{SysfsDriver::parse_properties_json(plugin_name(), drm_sysfs_json())}
        , M_DRM_RESOURCE_BY_GPU_TILE(load_resources_by_gpu_tile(
                                     drm_directory,
                                     (topo.num_domain(GEOPM_DOMAIN_GPU) > 0) ? 
                                     (topo.num_domain(GEOPM_DOMAIN_GPU_CHIP) /
                                     topo.num_domain(GEOPM_DOMAIN_GPU)) : 0))
        , m_domain(GEOPM_DOMAIN_GPU_CHIP)
    {
    }

    int DrmSysfsDriver::domain_type(const std::string &) const
    {
        return m_domain;
    }

    std::string DrmSysfsDriver::attribute_path(const std::string &name,
                                               int domain_idx)
    {
        auto resource_it = M_DRM_RESOURCE_BY_GPU_TILE.find(domain_idx);
        if (resource_it == M_DRM_RESOURCE_BY_GPU_TILE.end()) {
            throw Exception("DrmSysfsDriver::attribute_path(): domain_idx " + std::to_string(domain_idx) + " does not have a drm entry.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        auto property_it = M_PROPERTIES.find(name);
        if (property_it == M_PROPERTIES.end()) {
            throw Exception("DrmSysfsDriver::attribute_path(): No such signal " + name,
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        std::ostringstream oss;
        oss << resource_it->second
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
                result = static_cast<double>(std::stoi(content) * scaling_factor);
            }
            catch (const std::invalid_argument &ex) {}
            catch (const std::out_of_range &ex) {}
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
        return plugin_name();
    }

    std::map<std::string, SysfsDriver::properties_s> DrmSysfsDriver::properties(void) const
    {
        return M_PROPERTIES;
    }

    std::string DrmSysfsDriver::plugin_name(void)
    {
        return "DRM";
    }

    std::unique_ptr<IOGroup> DrmSysfsDriver::make_plugin(void)
    {
        return std::make_unique<SysfsIOGroup>(std::make_shared<DrmSysfsDriver>());
    }
}
