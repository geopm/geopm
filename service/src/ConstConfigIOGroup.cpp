/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "ConstConfigIOGroup.hpp"

#include <cmath>

#include <iostream>
#include <iterator>
#include <sstream>

#include "geopm_topo.h"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"

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
            {"values", {Json::ARRAY, false}},
            {"common_value", {Json::NUMBER, false}}
        };
    const std::string ConstConfigIOGroup::M_CONFIG_PATH_ENV =
        "GEOPM_CONST_CONFIG_PATH";
    const std::string ConstConfigIOGroup::M_DEFAULT_CONFIG_FILE_PATH =
        GEOPM_CONFIG_PATH "/const_config_io.json";

    ConstConfigIOGroup::ConstConfigIOGroup()
        : ConstConfigIOGroup(platform_topo(), geopm::get_env(M_CONFIG_PATH_ENV),
                             M_DEFAULT_CONFIG_FILE_PATH, hostname())
    {
    }

    ConstConfigIOGroup::ConstConfigIOGroup(const PlatformTopo &topo,
                                           const std::string &user_file_path,
                                           const std::string &default_file_path,
                                           const std::string &hostname)
        : m_platform_topo(topo)
        , M_THIS_HOST(hostname)
    {
        std::string config_json;
        bool load_default = true;
        if (!user_file_path.empty()) {
            try {
                config_json = geopm::read_file(user_file_path);
                load_default = false;
            }
            catch (const Exception &ex) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: <geopm> Failed to load "
                          << "ConstConfigIOGroup configuration file \""
                          << user_file_path << "\": " << ex.what()
                          << ". Proceeding with default configuration file..."
                          << std::endl;
#endif
            }
        }

        if (load_default) {
            config_json = geopm::read_file(default_file_path);
        }

        parse_config_json(config_json);
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
            result = it->second->domain;

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
        else if (domain_type != it->second->domain) {
            throw Exception("ConstConfigIOGroup::push_signal(): domain_type " +
                            std::to_string(domain_type) +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_idx < 0 ||
                 domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("ConstConfigIOGroup::push_signal(): domain_idx " +
                            std::to_string(domain_idx) + " out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        for (std::size_t i = 0; i < m_pushed_signals.size(); i++) {
            const m_signal_ref_s &signal = m_pushed_signals[i];
            if (it->second == signal.signal_info &&
                domain_idx == signal.domain_idx) {
                return static_cast<int>(i);
            }
        }

        if (it->second->is_common_value_provided) {
            domain_idx = 0;
        }

        m_pushed_signals.push_back({.signal_info = it->second,
                                    .domain_idx = domain_idx});

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

        if (batch_idx < 0 ||
            static_cast<std::size_t>(batch_idx) >= m_pushed_signals.size()) {
            throw Exception("ConstConfigIOGroup::sample(): batch_idx " +
                            std::to_string(batch_idx) + " out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        const m_signal_ref_s &signal = m_pushed_signals[batch_idx];
        result = signal.signal_info->values[signal.domain_idx];

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
        else if (domain_type != it->second->domain) {
            throw Exception("ConstConfigIOGroup::read_signal(): domain_type " +
                            std::to_string(domain_type) +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        else if (domain_idx < 0 ||
                 domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("ConstConfigIOGroup::read_signal(): domain_idx " +
                            std::to_string(domain_idx) + " out of range.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        double value = NAN;
        if (it->second->is_common_value_provided) {
            value = it->second->values[0];
        }
        else {
            value = it->second->values[domain_idx];
        }

        return value;
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
        return it->second->agg_function;
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

    std::string ConstConfigIOGroup::signal_description(const std::string &signal_name) const
    {
        auto it = m_signal_available.find(signal_name);
        if (it == m_signal_available.end()) {
            throw Exception("ConstConfigIOGroup::signal_description(): "
                            "signal_name " + signal_name +
                            " not valid for ConstConfigIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::string result;
        result =  "    description: " + it->second->description + '\n'; // Includes alias_for if applicable
        result += "    units: " + IOGroup::units_to_string(it->second->units) + '\n';
        result += "    aggregation: " + Agg::function_to_name(it->second->agg_function) + '\n';
        result += "    domain: " + m_platform_topo.domain_type_to_name(it->second->domain) + '\n';
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

        auto signals = root.object_items();
        for (const auto &signal : signals) {
            std::string name = signal.first;
            size_t at_idx = name.find('@');
            if (at_idx != std::string::npos) {
                std::string hostname = name.substr(at_idx + 1);
                if (hostname != M_THIS_HOST)
                    continue;
                name = name.substr(0, at_idx);
            }

            check_json_signal(signal);
            const std::string full_name = M_SIGNAL_PREFIX + name;

            auto properties = signal.second.object_items();
            int units = IOGroup::string_to_units(
                    properties["units"].string_value());
            int domain_type = PlatformTopo::domain_name_to_type(
                    properties["domain"].string_value());
            auto agg_func = Agg::name_to_function(
                    properties["aggregation"].string_value());

            std::vector<double> values;
            bool values_provided = properties.find("values") != properties.end();
            bool is_common_value_provided = properties.find("common_value") != properties.end();
            if (values_provided && is_common_value_provided) {
                // Only one field is required
                throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                "\"values\" and \"common_value\" provided for "
                                "signal \"" + name + "\"", GEOPM_ERROR_INVALID,
                                 __FILE__, __LINE__);
            }
            else if (!values_provided && !is_common_value_provided) {
                // One of the two fields is required
                throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                "missing \"values\" and \"common_value\" for "
                                "signal \"" + name + "\"", GEOPM_ERROR_INVALID,
                                 __FILE__, __LINE__);
            }

            if (values_provided) {
                auto json_values = properties["values"].array_items();
                if (json_values.empty()) {
                    throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                    "empty array of values provided for "
                                    "signal \"" + name + "\"", GEOPM_ERROR_INVALID,
                                    __FILE__, __LINE__);
                }
                if (static_cast<int>(json_values.size()) != m_platform_topo.num_domain(domain_type)) {
                    throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                    "number of values for signal \"" + name +
                                    "\" does not match domain size",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }

                for (const auto &val : json_values) {
                    if (!val.is_number()) {
                        throw Exception("ConstConfigIOGroup::parse_config_json():"
                                        " for signal \"" + name + "\", incorrect "
                                        "type for property: \"values\"",
                                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                    }
                    values.push_back(val.number_value());
                }
            }
            else {
                values.push_back(properties["common_value"].number_value());
            }

            std::string description = properties["description"].string_value();
            if (description.empty()) {
                throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                "empty description provided for signal \"" +
                                name + "\"", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
            }
            // TODO: check for duplicate signals. At the moment, we're using
            // json11 to parse JSON strings, which handles duplicate entries
            // by taking the latest entry encountered.
            m_signal_available[full_name] =
                std::make_shared<m_signal_info_s>(
                    m_signal_info_s {
                        .units = units,
                        .domain = domain_type,
                        .agg_function = agg_func,
                        .description = description,
                        .is_common_value_provided = is_common_value_provided,
                        .values = values});
        }
    }

    Json ConstConfigIOGroup::construct_config_json_obj(const std::string &config)
    {
        std::string err;
        Json root = Json::parse(config, err);
        if (!err.empty()) {
            throw Exception("ConstConfigIOGroup::parse_config_json(): "
                            "detected a malformed JSON string",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!root.is_object()) {
            throw Exception("ConstConfigIOGroup::parse_config_json(): "
                            "the root must be an object",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return root;
    }

    void ConstConfigIOGroup::check_json_signal(const Json::object::value_type &signal)
    {
        if (!signal.second.is_object()) {
            throw Exception("ConstConfigIOGroup::parse_config_json(): "
                            "invalid signal: \"" + signal.first + "\" ("
                            "object expected)", GEOPM_ERROR_INVALID,
                            __FILE__, __LINE__);
        }

        const std::string &signal_name = signal.first;
        const Json::object &properties = signal.second.object_items();
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
                throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                "for signal \"" + signal.first + "\", "
                                "unexpected property: \"" + prop.first +
                                "\"", GEOPM_ERROR_INVALID, __FILE__,
                                __LINE__);
            }
            else if (prop.second.type() != it->second.type) {
                throw Exception("ConstConfigIOGroup::parse_config_json(): "
                                "for signal \"" + signal.first + "\", "
                                "incorrect type for property: \"" +
                                prop.first + "\"", GEOPM_ERROR_INVALID,
                                __FILE__, __LINE__);
            }
            else {
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
}
