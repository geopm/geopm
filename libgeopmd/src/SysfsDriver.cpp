/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "SysfsDriver.hpp"

#include "geopm/json11.hpp"

#include "geopm/Agg.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"

using json11::Json;

namespace geopm
{
    static void check_json_type(const json11::Json &object,
                                const std::string &object_name,
                                enum json11::Json::Type expected_type)
    {
        bool is_valid = false;
        std::string expected_type_str = "";

        switch (expected_type) {
            case Json::BOOL:
                is_valid = object.is_bool();
                expected_type_str = "boolean";
                break;
            case Json::STRING:
                is_valid = object.is_string();
                expected_type_str = "string";
                break;
            case Json::NUMBER:
                is_valid = object.is_number();
                expected_type_str = "number";
                break;
            case Json::NUL:
                is_valid = object.is_null();
                expected_type_str = "null";
                break;
            case Json::ARRAY:
                is_valid = object.is_array();
                expected_type_str = "array";
                break;
            case Json::OBJECT:
                is_valid = object.is_object();
                expected_type_str = "object";
                break;
        }

        if (!is_valid) {
            throw Exception("SysfsDriver:" + object_name +
                                " JSON properties are malformed. Expected type: " +
                                expected_type_str,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    std::map<std::string, SysfsDriver::properties_s> SysfsDriver::parse_properties_json(const std::string &iogroup_name, const std::string &properties_json)
    {
        std::map<std::string, SysfsDriver::properties_s> result;
        std::string err;
        Json root = Json::parse(properties_json, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("SysfsDriver::" + std::string(__func__) +
                            "(): detected a malformed JSON string: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!root.has_shape({{"attributes", Json::OBJECT}}, err)) {
            throw Exception("SysfsDriver::" + std::string(__func__) +
                            "(): root of JSON string is malformed: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        const auto& attribute_object = root["attributes"].object_items();
        for (const auto &property_json : attribute_object) {
            const auto &property_name = iogroup_name + "::" + property_json.first;
            const auto &properties = property_json.second;


            check_json_type(properties["writeable"], property_name + ".writable", Json::BOOL);
            check_json_type(properties["attribute"], property_name + ".attribute", Json::STRING);
            check_json_type(properties["description"], property_name + ".description", Json::STRING);
            check_json_type(properties["scalar"], property_name + ".scalar", Json::NUMBER);
            check_json_type(properties["units"], property_name + ".units", Json::STRING);
            check_json_type(properties["aggregation"], property_name + ".aggregation", Json::STRING);
            check_json_type(properties["behavior"], property_name + ".behavior", Json::STRING);
            check_json_type(properties["format"], property_name + ".format", Json::STRING);
            check_json_type(properties["alias"], property_name + ".alias", Json::STRING);
            if (!properties["doc_domain"].is_null()) {
                check_json_type(properties["doc_domain"], property_name + ".doc_domain", Json::STRING);
            }

            result[property_name] = properties_s {
                    .name = property_name,
                    .is_writable = properties["writeable"].bool_value(),
                    .attribute = properties["attribute"].string_value(),
                    .description = properties["description"].string_value(),
                    .scaling_factor = properties["scalar"].number_value(),
                    .units = IOGroup::string_to_units(properties["units"].string_value()),
                    .aggregation_function = Agg::name_to_function(properties["aggregation"].string_value()),
                    .behavior = IOGroup::string_to_behavior(properties["behavior"].string_value()),
                    .format_function = geopm::string_format_name_to_function(properties["format"].string_value()),
                    .alias = properties["alias"].string_value()
                };
        }
        return result;
    }
}
