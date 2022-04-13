/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FilePolicy.hpp"

#include <cmath>

#include <algorithm>

#include "geopm/json11.hpp"

#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "config.h"

using json11::Json;

namespace geopm
{
    FilePolicy::FilePolicy(const std::string &policy_path,
                           const std::vector<std::string> &policy_names)
        : m_policy_path(policy_path)
        , m_policy_names(policy_names)
    {
        get_policy();
    }

    std::map<std::string, double> FilePolicy::parse_json(const std::string &path)
    {
        std::map<std::string, double> policy_value_map;
        std::string json_str;

        json_str = read_file(path);

        // Begin JSON parse
        std::string err;
        Json root = Json::parse(json_str, err);
        if (!err.empty() || !root.is_object()) {
            throw Exception("FilePolicy::" + std::string(__func__) + "(): detected a malformed json config file: " + err,
                            GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
        }

        for (const auto &obj : root.object_items()) {
            if (std::find(m_policy_names.begin(), m_policy_names.end(), obj.first) == m_policy_names.end()) {
                throw Exception("FilePolicy::" + std::string(__func__) + "(): invalid policy name: " +
                                obj.first, GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (obj.second.type() == Json::NUMBER) {
                policy_value_map.emplace(obj.first, obj.second.number_value());
            }
            else if (obj.second.type() == Json::STRING) {
                std::string tmp_val = obj.second.string_value();
                if (tmp_val.compare("NAN") == 0 || tmp_val.compare("NaN") == 0 || tmp_val.compare("nan") == 0) {
                    policy_value_map.emplace(obj.first, NAN);
                }
                else {
                    throw Exception("FilePolicy::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                    GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
                }
            }
            else {
                throw Exception("FilePolicy::" + std::string(__func__)  + ": unsupported type or malformed json config file",
                                GEOPM_ERROR_FILE_PARSE, __FILE__, __LINE__);
            }
        }

        return policy_value_map;
    }

    std::vector<double> FilePolicy::get_policy(void)
    {
        if (m_policy_names.size() > 0 && m_policy.size() == 0) {
            std::map<std::string, double> policy_value_map = parse_json(m_policy_path);
            for (auto name : m_policy_names) {
                auto it = policy_value_map.find(name);
                if (it != policy_value_map.end()) {
                    m_policy.emplace_back(policy_value_map.at(name));
                }
                else {
                    // Fill in missing policy values with NAN (default)
                    m_policy.emplace_back(NAN);
                }
            }
        }
        return m_policy;
    }
}
