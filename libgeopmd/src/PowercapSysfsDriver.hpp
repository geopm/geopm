/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POWERCAPSYSFSDRIVER_HPP_INCLUDE
#define POWERCAPSYSFSDRIVER_HPP_INCLUDE

#include "SysfsDriver.hpp"
#include "geopm/IOGroup.hpp"

namespace geopm
{
    class PowercapSysfsDriver : public SysfsDriver
    {
        public:
            PowercapSysfsDriver();
            PowercapSysfsDriver(const std::string &powercap_directory);
            virtual ~PowercapSysfsDriver() = default;
            int domain_type(const std::string &name) const override;
            std::string attribute_path(const std::string &name, int domain_idx) override;
            std::function<double(const std::string &)> signal_parse(const std::string &signal_name) const override;
            std::function<std::string(double)> control_gen(const std::string &control_name) const override;
            std::string driver(void) const override;
            std::map<std::string, properties_s> properties(void) const override;
            static std::string plugin_name(void);
            static std::unique_ptr<IOGroup> make_plugin(void);

        private:
            const std::map<std::string, properties_s> M_PROPERTIES;
            const std::map<std::string, std::string> M_POWERCAP_RESOURCE_BY_NAME;
            std::map<std::string, int> m_domain_map;
            const std::string m_powercap_directory;
    };
}

#endif
