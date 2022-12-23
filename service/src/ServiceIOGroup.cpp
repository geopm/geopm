/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "ServiceIOGroup.hpp"

#include <cmath>
#include <climits>
#include <cstring>
#include <unistd.h>
#include "geopm/Agg.hpp"
#include "geopm/ServiceProxy.hpp"
#include "BatchClient.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm_debug.hpp"

namespace geopm
{

    const std::string ServiceIOGroup::M_PLUGIN_NAME = "SERVICE";

    ServiceIOGroup::ServiceIOGroup()
        : ServiceIOGroup(platform_topo(),
                         ServiceProxy::make_unique(),
                         nullptr)
    {

    }

    ServiceIOGroup::ServiceIOGroup(const PlatformTopo &platform_topo,
                                   std::shared_ptr<ServiceProxy> service_proxy,
                                   std::shared_ptr<BatchClient> batch_client_mock)
        : m_platform_topo(platform_topo)
        , m_service_proxy(std::move(service_proxy))
        , m_signal_info(service_signal_info(m_service_proxy))
        , m_control_info(service_control_info(m_service_proxy))
        , m_batch_client(std::move(batch_client_mock))
        , m_session_pid(-1)
        , m_is_batch_active(false)
    {
        m_session_pid = getpid();
        m_service_proxy->platform_open_session();
    }

    ServiceIOGroup::~ServiceIOGroup()
    {
        if (m_is_batch_active) {
            m_batch_client->stop_batch();
        }
        if (m_session_pid == getpid()) {
            m_service_proxy->platform_close_session();
        }
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
        }
        return result;
    }

    std::set<std::string> ServiceIOGroup::control_names(void) const
    {
        std::set<std::string> result;
        for (const auto &ci : m_control_info) {
            result.insert(ci.first);
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
        if (!is_valid_signal(signal_name)) {
            throw Exception("ServiceIOGroup::push_signal(): signal name \"" +
                            signal_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != signal_domain_type(signal_name)) {
            throw Exception("ServiceIOGroup::push_signal(): domain_type requested does not match the domain of the signal (" + signal_name + ").",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("ServiceIOGroup::push_signal(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string signal_name_strip = strip_plugin_name(signal_name);
        if (signal_name_strip.size() >= NAME_MAX) {
            throw Exception("ServiceIOGroup::push_signal(): signal_name: " + signal_name + " is too long",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        geopm_request_s request;
        request.domain_type = domain_type;
        request.domain_idx = domain_idx;
        request.name[NAME_MAX - 1] = '\0';
        strncpy(request.name, signal_name_strip.c_str(), NAME_MAX - 1);
        m_signal_requests.push_back(request);
        return m_signal_requests.size() - 1;
    }

    int ServiceIOGroup::push_control(const std::string &control_name,
                                     int domain_type,
                                     int domain_idx)
    {
        if (!is_valid_control(control_name)) {
            throw Exception("ServiceIOGroup::push_control(): control name \"" +
                            control_name + "\" not found",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_type != control_domain_type(control_name)) {
            throw Exception("ServiceIOGroup::push_control(): domain_type requested does not match the domain of the control (" + control_name + ").",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (domain_idx < 0 || domain_idx >= m_platform_topo.num_domain(domain_type)) {
            throw Exception("ServiceIOGroup::push_control(): domain_idx out of range",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::string control_name_strip = strip_plugin_name(control_name);
        if (control_name_strip.size() >= NAME_MAX) {
            throw Exception("ServiceIOGroup::push_control(): control_name: " + control_name + " is too long",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        geopm_request_s request;
        request.domain_type = domain_type;
        request.domain_idx = domain_idx;
        request.name[NAME_MAX - 1] = '\0';
        strncpy(request.name, control_name_strip.c_str(), NAME_MAX - 1);
        m_control_requests.push_back(request);
        return m_control_requests.size() - 1;
    }

    void ServiceIOGroup::read_batch(void)
    {
        init_batch_server();
        if (m_is_batch_active &&
            m_signal_requests.size() != 0) {
            m_batch_samples = m_batch_client->read_batch();
        }
    }

    void ServiceIOGroup::write_batch(void)
    {
        if (m_is_batch_active &&
            m_control_requests.size() != 0) {
            m_batch_client->write_batch(m_batch_settings);
        }
    }

    double ServiceIOGroup::sample(int sample_idx)
    {
        if (m_signal_requests.size() == 0) {
            throw Exception("ServiceIOGroup::sample() called prior to any calls to push_signal()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_batch_samples.size() == 0) {
            throw Exception("ServiceIOGroup::sample() called prior to any calls to read_batch()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (sample_idx < 0 || (unsigned)sample_idx >= m_batch_samples.size()) {
            throw Exception("ServiceIOGroup::sample() called with parameter that was not returned by push_signal()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_batch_samples[sample_idx];
    }

    void ServiceIOGroup::adjust(int control_idx,
                                double setting)
    {
        if (m_control_requests.size() == 0) {
            throw Exception("ServiceIOGroup::adjust() called prior to any calls to push_control()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        init_batch_server();
        if (control_idx < 0 || (unsigned)control_idx >= m_batch_settings.size()) {
            throw Exception("ServiceIOGroup::adjust() called with an initial parameter that was not returned by push_control()",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_batch_settings[control_idx] = setting;
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
        // Implementation not required as ServiceIOGroup works with the service, which manages
        // sessions and saving controls.
    }

    void ServiceIOGroup::restore_control(void)
    {
        m_service_proxy->platform_restore_control();
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

    void ServiceIOGroup::init_batch_server(void)
    {
        if (!m_is_batch_active &&
            (m_signal_requests.size() != 0 ||
             m_control_requests.size() != 0)) {
            int server_pid = 0;
            std::string server_key;
            m_service_proxy->platform_start_batch(m_signal_requests,
                                                  m_control_requests,
                                                  server_pid,
                                                  server_key);
            if (m_batch_client == nullptr) {
                // Not a unit test
                m_batch_client = BatchClient::make_unique(server_key,
                                                          1.0,
                                                          m_signal_requests.size(),
                                                          m_control_requests.size());
            }
            m_is_batch_active = true;
            m_batch_settings.resize(m_control_requests.size(), NAN);
        }
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

    std::string ServiceIOGroup::name(void) const
    {
        return plugin_name();
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
