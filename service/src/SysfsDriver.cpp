/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SysfsDriver.hpp"
#include "geopm/json11.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"

using json11::Json;

namespace geopm
{
    std::map<std::string, SysfsDriver::properties_s> SysfsDriver::parse_properties_json(const std::string &iogroup_name, const std::string &properties_json)
    {
        std::map<std::string, SysfsDriver::properties_s> result;
        std::string err;
        Json root = Json::parse(properties_json, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("SysfsDriver::" + std::string(__func__) +
                            "(): detected a malformed json string: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (!root.has_shape({{"attributes", Json::OBJECT}}, err)) {
            throw Exception("SysfsDriver::" + std::string(__func__) +
                            "(): root of json string is malformed: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        const auto& attribute_object = root["attributes"].object_items();
        for (const auto &property_json : attribute_object) {
            const auto &property_name = iogroup_name + "::" + property_json.first;
            const auto &properties = property_json.second;

            if (!properties.has_shape({
                        {"writeable", Json::BOOL},
                        {"attribute", Json::STRING},
                        {"description", Json::STRING},
                        {"scalar", Json::NUMBER},
                        {"units", Json::STRING},
                        {"aggregation", Json::STRING},
                        {"behavior", Json::STRING},
                        {"format", Json::STRING},
                        {"alias", Json::STRING},
                        }, err)) {
                throw Exception("SysfsDriver::" + std::string(__func__) +
                                "(): " + property_name +
                                " json properties are malformed: " + err,
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
