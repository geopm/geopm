/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "ServiceIOGroup.hpp"

#include <cmath>
#include "Agg.hpp"
#include "ServiceProxy.hpp"
#include "Helper.hpp"
#include "PlatformTopo.hpp"
#include "geopm_debug.hpp"

namespace geopm
{

    const std::string ServiceIOGroup::M_PLUGIN_NAME = "SERVICE";

    ServiceIOGroup::ServiceIOGroup()
        : ServiceIOGroup(platform_topo(),
                         ServiceProxy::make_unique())
    {

    }

    ServiceIOGroup::ServiceIOGroup(const PlatformTopo &platform_topo,
                                   std::shared_ptr<ServiceProxy> service_proxy)
        : m_platform_topo(platform_topo)
        , m_service_proxy(service_proxy)
        , m_signal_info(service_signal_info(m_service_proxy))
        , m_control_info(service_control_info(m_service_proxy))
    {

    }

    std::map<std::string, signal_info_s> ServiceIOGroup::service_signal_info(std::shared_ptr<ServiceProxy> service_proxy)
    {
        std::vector<std::string> signal_names;
        std::vector<std::string> control_names;
        std::map<std::string, signal_info_s> result;
        service_proxy->platform_get_user_access(signal_names, control_names);
        auto signal_info = service_proxy->platform_get_signal_info(signal_names);
        GEOPM_DEBUG_ASSERT(signal_info.size() == signal_names.size(),
                           "platform_get_signal_info() DBus interface returned the wrong size result");
        int num_signal = signal_names.size();
        for (int signal_idx = 0; signal_idx != num_signal; ++signal_idx) {
            result[signal_names[signal_idx]] = signal_info[signal_idx];
            result[M_PLUGIN_NAME + "::" + signal_names[signal_idx]] = signal_info[signal_idx];
        }
        return result;
    }

    std::map<std::string, control_info_s> ServiceIOGroup::service_control_info(std::shared_ptr<ServiceProxy> service_proxy)
    {
        std::vector<std::string> signal_names;
        std::vector<std::string> control_names;
        std::map<std::string, control_info_s> result;
        service_proxy->platform_get_user_access(signal_names, control_names);
        auto control_info = service_proxy->platform_get_control_info(control_names);
        GEOPM_DEBUG_ASSERT(control_info.size() == control_names.size(),
                           "platform_get_control_info() DBus interface returned the wrong size result");
        int num_control = control_names.size();
        for (int control_idx = 0; control_idx != num_control; ++control_idx) {
            result[control_names[control_idx]] = control_info[control_idx];
            result[M_PLUGIN_NAME + "::" + control_names[control_idx]] = control_info[control_idx];
        }
        return result;
    }

    std::set<std::string> ServiceIOGroup::signal_names(void) const
    {
        std::set<std::string> result;
        for (const auto &si : m_signal_info) {
            result.insert(si.first);
            result.insert(M_PLUGIN_NAME + "::" + si.first);
        }
        return result;
    }

    std::set<std::string> ServiceIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &ci : m_control_info) {
            result.insert(ci.first);
            result.insert(M_PLUGIN_NAME + "::" + ci.first);
        }
        return result;
    }

    bool ServiceIOGroup::is_valid_signal(const std::string &signal_name) const
    {
        return (m_signal_info.find(signal_name) != m_signal_info.end());
    }

    bool ServiceIOGroup::is_valid_control(const std::string &control_name) const
    {
        return (m_control_info.find(control_name) != m_control_info.end());
    }

    int ServiceIOGroup::signal_domain_type(const std::string &signal_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_signal_info.find(signal_name);
        if (it != m_signal_info.end()) {
            result = it->second.domain;
        }
        return result;
    }

    int ServiceIOGroup::control_domain_type(const std::string &control_name) const
    {
        int result = GEOPM_DOMAIN_INVALID;
        auto it = m_control_info.find(control_name);
        if (it != m_control_info.end()) {
            result = it->second.domain;
        }
        return result;
    }

    int ServiceIOGroup::push_signal(const std::string &signal_name,
                                    int domain_type,
                                    int domain_idx)
    {
        throw Exception("ServiceIOGroup::push_signal()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return -1;
    }

    int ServiceIOGroup::push_control(const std::string &control_name,
                                     int domain_type,
                                     int domain_idx)
    {
        throw Exception("ServiceIOGroup::push_control()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return -1;
    }

    void ServiceIOGroup::read_batch(void)
    {
        throw Exception("ServiceIOGroup::read_batch()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void ServiceIOGroup::write_batch(void)
    {
        throw Exception("ServiceIOGroup::write_batch()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ServiceIOGroup::sample(int sample_idx)
    {
        throw Exception("ServiceIOGroup::sample()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        return NAN;
    }

    void ServiceIOGroup::adjust(int control_idx,
                                double setting)
    {
        throw Exception("ServiceIOGroup::adjust()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ServiceIOGroup::read_signal(const std::string &signal_name,
                                       int domain_type,
                                       int domain_idx)
    {
        if (!is_valid_signal(signal_name)) {
            throw Exception("ServiceIOGroup::read_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("ServiceIOGroup::read_signal(): domain_type requested does not match the domain of the signal (" + signal_name + ").",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("ServiceIOGroup::read_signal(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string signal_name_strip = strip_plugin_name(signal_name);
        return m_service_proxy->platform_read_signal(signal_name_strip, domain_type, domain_idx);
    }

    void ServiceIOGroup::write_control(const std::string &control_name,
                                       int domain_type,
                                       int domain_idx,
                                       double setting)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("ServiceIOGroup::write_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("ServiceIOGroup::write_control(): domain_type does not match the domain of the control.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("ServiceIOGroup::write_control(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string control_name_strip = strip_plugin_name(control_name);
        m_service_proxy->platform_write_control(control_name_strip, domain_type, domain_idx, setting);
    }

    void ServiceIOGroup::save_control(void)
    {
        // Proxy, so no direct save/restore
    }

    void ServiceIOGroup::restore_control(void)
    {
        // Proxy, so no direct save/restore
    }

    std::function<double(const std::vector<double> &)> ServiceIOGroup::agg_function(const std::string &signal_name) const
    {
        auto it = m_signal_info.find(signal_name);
        if (it == m_signal_info.end()) {
            throw Exception("ServiceIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for ServiceIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return Agg::type_to_function(it->second.aggregation);
    }

    std::function<std::string(double)> ServiceIOGroup::format_function(const std::string &signal_name) const
    {
        auto it = m_signal_info.find(signal_name);
        if (it == m_signal_info.end()) {
            throw Exception("ServiceIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for ServiceIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return string_format_type_to_function(it->second.string_format);
    }

    std::string ServiceIOGroup::signal_description(const std::string &signal_name) const
    {
        auto it = m_signal_info.find(signal_name);
        if (it == m_signal_info.end()) {
            throw Exception("ServiceIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for ServiceIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.description;
    }

    std::string ServiceIOGroup::control_description(const std::string &control_name) const
    {
        auto it = m_control_info.find(control_name);
        if (it == m_control_info.end()) {
            throw Exception("ServiceIOGroup::control_behavior(): control_name " + control_name +
                            " not valid for ServiceIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.description;
    }

    int ServiceIOGroup::signal_behavior(const std::string &signal_name) const
    {
        auto it = m_signal_info.find(signal_name);
        if (it == m_signal_info.end()) {
            throw Exception("ServiceIOGroup::signal_behavior(): signal_name " + signal_name +
                            " not valid for ServiceIOGroup",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second.behavior;
    }

    void ServiceIOGroup::save_control(const std::string &save_path)
    {
        // Proxy, so no direct save/restore
    }

    void ServiceIOGroup::restore_control(const std::string &save_path)
    {
        // Proxy, so no direct save/restore
    }

    std::string ServiceIOGroup::strip_plugin_name(const std::string &name)
    {
        static const std::string key = M_PLUGIN_NAME + "::";
        static const size_t key_len = key.size();
        std::string result = name;
        if (string_begins_with(name, key)) {
            result = name.substr(key_len);
        }
        return result;
    }

    std::string ServiceIOGroup::plugin_name(void)
    {
        return M_PLUGIN_NAME;
    }

    std::unique_ptr<IOGroup> ServiceIOGroup::make_plugin(void)
    {
        return geopm::make_unique<ServiceIOGroup>();
    }
}
