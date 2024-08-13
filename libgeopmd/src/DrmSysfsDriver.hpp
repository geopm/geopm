/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DRMSYSFSDRIVER_HPP_INCLUDE
#define DRMSYSFSDRIVER_HPP_INCLUDE

#include <map>
#include <vector>

#include "geopm_topo.h"

#include "SysfsDriver.hpp"

namespace geopm
{
    class IOGroup;
    class PlatformTopo;

    /// @brief Class used to implement the DrmSysfsDriverGroup
    class DrmSysfsDriver: public SysfsDriver
    {
        public:
            DrmSysfsDriver();
            DrmSysfsDriver(const PlatformTopo &topo,
                           const std::string &drm_directory);
            virtual ~DrmSysfsDriver() = default;
            int domain_type(const std::string &name) const override;
            std::string attribute_path(const std::string &name,
                                       int domain_idx) override;
            std::function<double(const std::string&)> signal_parse(const std::string &signal_name) const override;
            std::function<std::string(double)> control_gen(const std::string &control_name) const override;
            std::string driver(void) const override;
            std::map<std::string, SysfsDriver::properties_s> properties(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);
        private:
            const std::map<std::string, SysfsDriver::properties_s> M_PROPERTIES;
            const std::map<std::pair<geopm_domain_e, int>, std::string> M_DRM_HWMON_DIR_BY_GEOPM_DOMAIN;
            const std::map<int, std::string> M_DRM_RESOURCE_BY_GPU_TILE;
    };
}

#endif
