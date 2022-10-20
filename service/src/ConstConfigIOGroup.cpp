/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ConstConfigIOGroup.hpp"

#include <cmath>

#include <iostream>
#include <iterator>
#include <sstream>

#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "geopm_topo.h"
#include "config.h"

using json11::Json;

namespace geopm
{
    const std::string ConstConfigIOGroup::M_PLUGIN_NAME = "CONST_CONFIG";
    const std::string ConstConfigIOGroup::M_SIGNAL_PREFIX = M_PLUGIN_NAME + "::";
    const std::map<std::string, ConstConfigIOGroup::m_signal_desc_s>
        ConstConfigIOGroup::M_SIGNAL_FIELDS = {
            {"description", {Json::STRING, true}},
            {"units", {Json::STRING, true}},
            {"domain", {Json::STRING, true}},
            {"aggregation", {Json::STRING, true}},
            {"values", {Json::ARRAY, true}}
        };

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
        auto it = m_signal_available.find(signal_name);

        if (it != m_signal_available.end())
            result = it->second.domain;

        return result;
    }

    int ConstConfigIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int ConstConfigIOGroup::push_signal(const std::string &signal_name,
                                        int domain_type, int domain_idx)
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::push_signal(): " +
                            signal_name + " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != it->second.domain) {
            throw Exception("ConstConfigIOGroup::push_signal(): domain_type " +
                            std::to_string(domain_type) +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_idx < 0 || domain_idx >= it->second.values.size()) {
            throw Exception("ConstConfigIOGroup::push_signal(): domain_idx " +
                            std::to_string(domain_idx) + " out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        size_t signal_idx = std::distance(m_signal_available.begin(), it);
        bool is_found = false;
        for (auto const &signal : m_pushed_signals) {
            if (signal_idx == signal.signal_idx &&
                domain_idx == signal.domain_idx) {
                is_found = true;
                break;
            }
        }

        if (!is_found) {
            m_signal_ref_s signal = {
                .signal_idx = signal_idx,
                .domain_idx = domain_idx
            };
            m_pushed_signals.push_back(signal);
        }
        
        return m_pushed_signals.size() - 1;
    }

    int ConstConfigIOGroup::push_control(const std::string &control_name,
                                         int domain_type, int domain_idx)
    {
        throw Exception("ConstConfigIOGroup::push_control(): there are no "
                        "controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ConstConfigIOGroup::read_batch(void)
    {
    }

    void ConstConfigIOGroup::write_batch(void)
    {
    }

    double ConstConfigIOGroup::sample(int batch_idx)
    {
        double result = NAN;
        
        if (batch_idx < 0 || batch_idx >= m_pushed_signals.size()) {
            throw Exception("ConstConfigIOGroup::sample(): batch_idx " +
                            std::to_string(batch_idx) + " out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_signal_ref_s signal = m_pushed_signals[batch_idx];
        auto it = m_signal_available.begin();
        std::advance(it, signal.signal_idx);
        result = it->second.values[signal.domain_idx];

        return result;
    }

    void ConstConfigIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("ConstConfigIOGroup::adjust(): there are no controls "
                        "supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double ConstConfigIOGroup::read_signal(const std::string &signal_name,
                                           int domain_type, int domain_idx)
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::read_signal(): " +
                            signal_name + " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_type != it->second.domain) {
            throw Exception("ConstConfigIOGroup::read_signal(): domain_type " +
                            std::to_string(domain_type) +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_idx < 0 || domain_idx >= it->second.values.size()) {
            throw Exception("ConstConfigIOGroup::read_signal(): domain_idx " +
                            std::to_string(domain_idx) + " out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return it->second.values[domain_idx];
    }

    void ConstConfigIOGroup::write_control(const std::string &control_name,
                                           int domain_type, int domain_idx,
                                           double setting)
    {
        throw Exception("ConstConfigIOGroup::write_control(): there are no "
                        "controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void ConstConfigIOGroup::save_control(void)
    {
    }

    void ConstConfigIOGroup::restore_control(void)
    {
    }

    std::function<double(const std::vector<double> &)>
    ConstConfigIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::agg_function(): unknown how "
                            "to aggregate \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.agg_function;
    }

    std::function<std::string(double)>
    ConstConfigIOGroup::format_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ConstConfigIOGroup::format_function(): unknown "
                            "how to format \"" + signal_name + "\"",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return string_format_double;
    }

    std::string ConstConfigIOGroup::signal_description(
        const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::signal_description(): "
                            "signal_name " + signal_name +
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

    std::string ConstConfigIOGroup::control_description(
        const std::string &control_name) const
    {
        throw Exception("ConstConfigIOGroup::control_description: there are "
                        "no controls supported by the ConstConfigIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    int ConstConfigIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ConstConfigIOGroup::signal_behavior(): "
                            "signal_name " + signal_name +
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
        return M_PLUGIN_NAME;
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
        Json root = construct_config_json_obj(config);
        check_json_root(root);

        auto signals = root.object_items();
        for (const auto &signal : signals) {
            check_json_signal(signal);
            std::string name = M_SIGNAL_PREFIX + signal.first;

            auto properties = signal.second.object_items();
            check_json_signal_properties(signal.first, properties);

            int units = IOGroup::string_to_units(
                    properties["units"].string_value());
            int domain_type = PlatformTopo::domain_name_to_type(
                    properties["domain"].string_value());
            auto agg_func = Agg::name_to_function(
                    properties["aggregation"].string_value());

            auto json_values = properties["values"].array_items();
            std::vector<double> values;
            for (const auto &val : json_values) {
                check_json_array_value(val);
                values.push_back(val.number_value());
            }

            std::string description = properties["description"].string_value();
            m_signal_available[name] = {
                .units = units,
                .domain = domain_type,
                .agg_function = agg_func,
                .description = description,
                .values = values
            };
        }
    }

    Json ConstConfigIOGroup::construct_config_json_obj(
        const std::string &config)
    {
        std::string err;
        Json root = Json::parse(config, err);
        if (!err.empty()) {
            throw Exception("ConstConfigIOGroup::parse_config_json(): "
                            "detected a malformed JSON string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return root;
    }

    void ConstConfigIOGroup::check_json_root(const Json &root)
    {
        if (!root.is_object()) {
            throw Exception("ConstConfigIOGroup::parse_config_json(): "
                            "the root must be an object",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }


    void ConstConfigIOGroup::check_json_signal(
        const Json::object::value_type &signal_obj)
    {
        if (!signal_obj.second.is_object()) {
            throw Exception("ConstConfigIOGroup::parse_config_json(): "
                            "invalid signal: \"" + signal_obj.first + "\" ("
                            "object expected)", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }
    }

    void ConstConfigIOGroup::check_json_signal_properties(
        const std::string &signal_name,
        const Json::object& properties)
    {
        // property -> found
        std::map<std::string, bool> required_keys;
        for (const auto &field : M_SIGNAL_FIELDS) {
            if (field.second.required)
                required_keys[field.first] = false;
        }

        size_t properties_found = 0;
        for (const auto &prop : properties) {
            auto it = M_SIGNAL_FIELDS.find(prop.first);
            if (it == M_SIGNAL_FIELDS.end()) {
                throw Exception("ConstConfigIOGroup::parse_config_json():"
                                " unexpected propety: \"" + prop.first +
                                "\"", GEOPM_ERROR_INVALID, __FILE__,
                                __LINE__);
            } else if (prop.second.type() != it->second.type) {
                throw Exception("ConstConfigIOGroup::parse_config_json():"
                                " incorrect type for property: \"" +
                                prop.first + "\"", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
            } else {
                properties_found++;
                required_keys[prop.first] = true;
            }
        }
        size_t missing = required_keys.size() - properties_found;
        if (missing > 0) {
            std::ostringstream err_msg;
            size_t added = 0;
            err_msg << "missing properties for signal \"" << signal_name
                    << "\": ";
            for (const auto &prop : required_keys) {
                if (!prop.second) {
                    err_msg << prop.first;
                    added++;
                    if (added < missing)
                        err_msg << ", ";
                }
            }
            throw Exception("ConstConfigIOGroup::parse_config_json(): " +
                            err_msg.str(), GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }
    }

    void ConstConfigIOGroup::check_json_array_value(const Json& val)
    {
        if (!val.is_number()) {
            throw Exception("ConstConfigIOGroup::parse_config_json():"
                            " incorrect type for property: "
                            "\"values\"", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }
    }
}
