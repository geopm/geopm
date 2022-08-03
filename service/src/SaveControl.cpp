/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SaveControl.hpp"

#include "geopm/json11.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/IOGroup.hpp"
#include "geopm/PlatformTopo.hpp"

using json11::Json;

namespace geopm
{
    std::unique_ptr<SaveControl>
    SaveControl::make_unique(const std::vector<m_setting_s> &settings)
    {
        return geopm::make_unique<SaveControlImp>(settings);
    }

    std::unique_ptr<SaveControl>
    SaveControl::make_unique(const std::string &json_string)
    {
        return geopm::make_unique<SaveControlImp>(json_string);
    }

    std::unique_ptr<SaveControl>
    SaveControl::make_unique(IOGroup &io_group)
    {
        return geopm::make_unique<SaveControlImp>(io_group, platform_topo());
    }

    SaveControlImp::SaveControlImp(const std::vector<m_setting_s> &settings)
        : m_settings(settings)
    {

    }

    SaveControlImp::SaveControlImp(const std::string &json_string)
        : m_json(json_string)
    {

    }

    SaveControlImp::SaveControlImp(IOGroup &io_group, const PlatformTopo &topo)
        : m_settings(settings(io_group, topo))
    {

    }

    std::string SaveControlImp::json(void) const
    {
        std::string result = m_json;
        if (result.empty()) {
            result = json(m_settings);
        }
        return result;
    }

    std::vector<SaveControl::m_setting_s> SaveControlImp::settings(void) const
    {
        std::vector<m_setting_s> result = m_settings;
        if (result.empty()) {
            result = settings(m_json);
        }
        return result;
    }


    std::string SaveControlImp::json(const std::vector<m_setting_s> &settings)
    {
        std::vector<std::map<std::string, Json> > json_settings;
        for (const auto &ss : settings) {
            json_settings.push_back({{"name", ss.name},
                {"domain_type", ss.domain_type},
                {"domain_idx", ss.domain_idx},
                {"setting", ss.setting}});
        }
        Json json_obj(json_settings);
        return json_obj.dump();
    }

    std::vector<SaveControl::m_setting_s>
    SaveControlImp::settings(const std::string &json_string)
    {
        std::vector<m_setting_s> result;
        std::string err;
        Json root = Json::parse(json_string, err);
        if (!err.empty() || !root.is_array()) {
            throw Exception("SaveControlImp::settings(): Expected a JSON array, unable to parse: " + err,
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        for (const auto &jss : root.array_items()) {
            if (!jss.is_object()) {
                throw Exception("SaveControlImp::settings(): Expected a JSON object, unable to parse",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            auto jss_map = jss.object_items();
            std::vector<std::string> required_keys = {"name",
                                                      "domain_type",
                                                      "domain_idx",
                                                      "setting"
                                                     };
            if (jss_map.size() != required_keys.size()) {
                throw Exception("SaveControlImp::Settins(): JSON object representing m_setting_s must have four fields",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            for (const auto &rk : required_keys) {
                if (jss_map.count(rk) == 0) {
                    throw Exception("SaveControlImp::settings(): Invalid settings object JSON, missing a required field: \"" + rk + "\"",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
            result.push_back(m_setting_s{jss["name"].string_value(),
                                         jss["domain_type"].int_value(),
                                         jss["domain_idx"].int_value(),
                                         jss["setting"].number_value()});
        }
        return result;
    }

    void SaveControlImp::write_json(const std::string &save_path) const
    {
        write_file(save_path, json());
    }

    void SaveControlImp::restore(IOGroup &io_group) const
    {
        for (const auto &ss : settings()) {
            io_group.write_control(ss.name,
                                   ss.domain_type,
                                   ss.domain_idx,
                                   ss.setting);
        }
    }

    std::vector<SaveControl::m_setting_s>
    SaveControlImp::settings(IOGroup &io_group,
                             const PlatformTopo &topo)
    {
        std::vector<m_setting_s> result;
        std::string prefix = io_group.name() + "::";
        for (const auto &name : io_group.control_names()) {
            if (string_begins_with(name, prefix)) {
                int dom_type = io_group.control_domain_type(name);
                int num_dom = topo.num_domain(dom_type);
                for (int dom_idx = 0; dom_idx != num_dom; ++dom_idx) {
                    double setting = io_group.read_signal(name,
                                                          dom_type,
                                                          dom_idx);
                    result.push_back({name,
                                      dom_type,
                                      dom_idx,
                                      setting});
                }
            }
        }
        return result;
    }
}
