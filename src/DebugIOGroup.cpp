/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DebugIOGroup.hpp"

#include "geopm/PlatformTopo.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Agg.hpp"
#include "config.h"

namespace geopm
{
    DebugIOGroup::DebugIOGroup(const PlatformTopo &topo,
                               std::shared_ptr<std::vector<double> > value_cache)
        : m_topo(topo)
        , m_value_cache(std::move(value_cache))
        , m_num_reg_signals(0)
    {
        if (m_value_cache == nullptr) {
            throw Exception("DebugIOGroup(): value_cache cannot be null.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void DebugIOGroup::register_signal(const std::string &name, int domain_type,
                                       int signal_behavior)
    {
        if (m_signal_name.find(name) != m_signal_name.end()) {
            throw Exception("DebugIOGroup::register_signal(): signal " + name + " already registered.",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_signal_info[name] = {domain_type, signal_behavior};
        int num_domain = m_topo.num_domain(domain_type);
        for (int idx = 0; idx < num_domain; ++idx) {
            if (m_num_reg_signals >= m_value_cache->size()) {
                throw Exception("DebugIOGroup::register_signal(): number of registered signals was greater than size of shared vector provided.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_signal_idx[std::make_pair(name, idx)] = m_num_reg_signals;
            ++m_num_reg_signals;
        }
        m_signal_name.insert(name);
    }

    std::set<std::string> DebugIOGroup::signal_names(void) const
    {
        return m_signal_name;
    }

    std::set<std::string> DebugIOGroup::control_names(void) const
    {
        return {};
    }

    bool DebugIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return m_signal_name.find(signal_name) != m_signal_name.end();
    }

    bool DebugIOGroup::is_valid_control(const std::string &control_name) const
    {
        return false;
    }

    int DebugIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        if (is_valid_signal(signal_name)) {
            result = m_signal_info.at(signal_name).domain_type;
        }
        return result;
    }

    int DebugIOGroup::control_domain_type(const std::string &control_name) const
    {
        return GEOPM_DOMAIN_INVALID;
    }

    int DebugIOGroup::push_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::push_signal(): signal_name " + signal_name +
                            " not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("DebugIOGroup::push_signal(): signal_name " + signal_name +
                            " not defined for domain " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx > m_topo.num_domain(domain_type)) {
            throw Exception("DebugIOGroup::push_signal(): domain index out of bounds "
                            "for domain" + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_idx.at({signal_name, domain_idx});
    }

    int DebugIOGroup::push_control(const std::string &control_name, int domain_type, int domain_idx)
    {
        throw Exception("DebugIOGroup::push_control(): there are no controls supported by the DebugIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void DebugIOGroup::read_batch(void)
    {

    }

    void DebugIOGroup::write_batch(void)
    {

    }

    double DebugIOGroup::sample(int batch_idx)
    {
        if (batch_idx < 0 || (size_t)batch_idx >= m_value_cache->size()) {
            throw Exception("DebugIOGroup::sample(): batch_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_value_cache->at(batch_idx);
    }

    void DebugIOGroup::adjust(int batch_idx, double setting)
    {
        throw Exception("DebugIOGroup::adjust(): there are no controls supported by the DebugIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    double DebugIOGroup::read_signal(const std::string &signal_name, int domain_type, int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup:read_signal(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("DebugIOGroup::read_signal(): signal_name " + signal_name +
                            " not defined for domain " + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx > m_topo.num_domain(domain_type)) {
            throw Exception("DebugIOGroup::read_signal(): domain index out of bounds "
                            "for domain" + std::to_string(domain_type),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_value_cache->at(m_signal_idx.at({signal_name, domain_idx}));
    }

    void DebugIOGroup::write_control(const std::string &control_name, int domain_type, int domain_idx, double setting)
    {

    }

    void DebugIOGroup::save_control(void)
    {

    }

    void DebugIOGroup::restore_control(void)
    {

    }

    std::string DebugIOGroup::name(void) const
    {
        return plugin_name();
    }

    std::string DebugIOGroup::plugin_name(void)
    {
        return "DEBUG";
    }

    std::unique_ptr<IOGroup> DebugIOGroup::make_plugin(void)
    {
        throw Exception("DebugIOGroup::make_plugin(): this IOGroup should not be created through factory.",
                        GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        return nullptr;
    }

    std::function<double(const std::vector<double> &)> DebugIOGroup::agg_function(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::agg_function(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return Agg::select_first;
    }

    std::string DebugIOGroup::signal_description(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::signal_description(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return "DebugIOGroup signals should only be used by an Agent.  "
               "No description is available.";
    }

    std::string DebugIOGroup::control_description(const std::string &control_name) const
    {
        throw Exception("DebugIOGroup::control_description(): there are no controls supported by the DebugIOGroup",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        return "";
    }

    int DebugIOGroup::signal_behavior(const std::string &signal_name) const
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("DebugIOGroup::signal_behavior(): " + signal_name +
                            "not valid for DebugIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_signal_info.at(signal_name).behavior;
    }

    void DebugIOGroup::save_control(const std::string &save_path)
    {

    }

    void DebugIOGroup::restore_control(const std::string &save_path)
    {

    }
}
