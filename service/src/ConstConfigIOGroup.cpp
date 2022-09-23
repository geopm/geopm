/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ConstConfigIOGroup.hpp"

#include <cmath>

#include <iostream>
#include <iterator>

#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm_topo.h"
#include "config.h"

#define GEOPM_CONSTCONFIG_IO_GROUP_PLUGIN_NAME "CONSTCONFIG"

namespace geopm
{
    ConstConfigIOGroup::ConstConfigIOGroup()
    {
        // TODO
    }

    ConstConfigIOGroup::ConstConfigIOGroup(const std::string &config)
    {
        parse_config_json(config);
    }

    std::set<std::string> ConstConfigIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &sv : m_signal_available) {
            result.insert(sv.first);
        }
        return result;
    }

    std::set<std::string> ConstConfigIOGroup::control_names(void) const
    {
        return {};
    }

    bool ConstConfigIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_available.find(signal_name) != m_signal_available.end();
    }

    bool ConstConfigIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int ConstConfigIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        // TODO
        return result;
    }

    int ConstConfigIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int ConstConfigIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ConstConfigIOGroup::push_signal(): " + signal_name +
                            "not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // TODO
        return 0;
    }

    int ConstConfigIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("ConstConfigIOGroup::push_control(): there are no controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ConstConfigIOGroup::read_batch(void)
    {
        // TODO: enforce proper behavior/sequence of calls: push_signal(),
        // read_batch(), sample()?
    }

    void ConstConfigIOGroup::write_batch(void)
    {
    }

    double ConstConfigIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        // TODO
        return result;
    }

    void ConstConfigIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("ConstConfigIOGroup::adjust(): there are no controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double ConstConfigIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ConstConfigIOGroup::read_signal(): " + signal_name +
                            "not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // TODO
        return NAN;
    }

    void ConstConfigIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {
        throw Exception("ConstConfigIOGroup::write_control(): there are no controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ConstConfigIOGroup::save_control(void)
    {
    }

    void ConstConfigIOGroup::restore_control(void)
    {
    }

    std::function<double(const std::vector<double> &)> ConstConfigIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::agg_function(): unknown how to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.agg_function;
    }

    std::function<std::string(double)> ConstConfigIOGroup::format_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ConstConfigIOGroup::format_function(): unknown how to format \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        // TODO?
        return string_format_double;
    }

    std::string ConstConfigIOGroup::signal_description(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::signal_description(): signal_name " + signal_name +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string result;
        result =  "    description: " + it->second.description + '\n'; // Includes alias_for if applicable
        result += "    units: " + IOGroup::units_to_string(it->second.units) + '\n';
        result += "    aggregation: " + Agg::function_to_name(it->second.agg_function) + '\n';
        result += "    domain: " + platform_topo().domain_type_to_name(GEOPM_DOMAIN_BOARD) + '\n';
        result += "    iogroup: ConstConfigIOGroup";
        return result;
    }

    std::string ConstConfigIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("ConstConfigIOGroup::control_description: there are no controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int ConstConfigIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ConstConfigIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return IOGroup::M_SIGNAL_BEHAVIOR_CONSTANT;
    }

    std::string ConstConfigIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::string ConstConfigIOGroup::plugin_name(void)
    {
        return GEOPM_CONSTCONFIG_IO_GROUP_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> ConstConfigIOGroup::make_plugin(void)
    {
        return geopm::make_unique<ConstConfigIOGroup>();
    }

    void ConstConfigIOGroup::save_control(const std::string &save_path)
    {
    }

    void ConstConfigIOGroup::restore_control(const std::string &save_path)
    {
    }

    void ConstConfigIOGroup::parse_config_json(const std::string &config)
    {
        // TODO
    }
}
