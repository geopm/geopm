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

#include "ServiceProxy.hpp"

#include <sstream>

#include "geopm/PlatformIO.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "SDBus.hpp"
#include "SDBusMessage.hpp"


namespace geopm
{

    std::unique_ptr<ServiceProxy> ServiceProxy::make_unique(void)
    {
        return geopm::make_unique<ServiceProxyImp>();
    }

    ServiceProxyImp::ServiceProxyImp()
        : ServiceProxyImp(SDBus::make_unique())
    {

    }

    ServiceProxyImp::ServiceProxyImp(std::shared_ptr<SDBus> bus)
        : m_bus(bus)
    {

    }

    void ServiceProxyImp::platform_get_user_access(std::vector<std::string> &signal_names,
                                                   std::vector<std::string> &control_names)
    {
        std::shared_ptr<SDBusMessage> bus_message =
            m_bus->call_method("PlatformGetUserAccess");

        bus_message->enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "asas");
        signal_names = read_string_array(bus_message);
        control_names = read_string_array(bus_message);
        bus_message->exit_container();
    }

    std::vector<signal_info_s> ServiceProxyImp::platform_get_signal_info(
        const std::vector<std::string> &signal_names)
    {
        std::vector<signal_info_s> result;
        std::shared_ptr<SDBusMessage> bus_message =
            m_bus->make_call_message("PlatformGetSignalInfo");
        bus_message->append_strings(signal_names);
        std::shared_ptr<SDBusMessage> bus_reply = m_bus->call_method(bus_message);
        bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_ARRAY, "(ssiiii)");
        bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "ssiiii");
        while (bus_reply->was_success()) {
            std::string name = bus_reply->read_string();
            std::string description = bus_reply->read_string();
            int domain = bus_reply->read_integer();
            int aggregation = bus_reply->read_integer();
            int string_format = bus_reply->read_integer();
            int behavior = bus_reply->read_integer();
            bus_reply->exit_container();
            result.push_back({name, description, domain, aggregation,
                              string_format, behavior});
            bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "ssiiii");
        }
        bus_reply->exit_container();
        return result;
    }

    std::vector<control_info_s> ServiceProxyImp::platform_get_control_info(
        const std::vector<std::string> &control_names)
    {
        std::vector<control_info_s> result;
        std::shared_ptr<SDBusMessage> bus_message =
            m_bus->make_call_message("PlatformGetControlInfo");
        bus_message->append_strings(control_names);
        std::shared_ptr<SDBusMessage> bus_reply = m_bus->call_method(bus_message);
        bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_ARRAY, "(ssi)");
        bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "ssi");
        while (bus_reply->was_success()) {
            std::string name = bus_reply->read_string();
            std::string description = bus_reply->read_string();
            int domain = bus_reply->read_integer();
            bus_reply->exit_container();
            result.push_back({name, description, domain});
            bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "ssi");
        }
        bus_reply->exit_container();
        return result;
    }

    void ServiceProxyImp::platform_open_session(void)
    {
        (void)m_bus->call_method("PlatformOpenSession");
    }

    void ServiceProxyImp::platform_close_session(void)
    {
        (void)m_bus->call_method("PlatformCloseSession");
    }

    void ServiceProxyImp::platform_start_batch(const std::vector<struct geopm_request_s> &signal_config,
                                               const std::vector<struct geopm_request_s> &control_config,
                                               int &server_pid,
                                               std::string &server_key)
    {
        std::shared_ptr<SDBusMessage> bus_message = m_bus->make_call_message("PlatformStartBatch");
        bus_message->open_container(SDBusMessage::M_MESSAGE_TYPE_ARRAY, "(iis)");
        for (const auto &sig_it : signal_config) {
            bus_message->append_request_s(sig_it);
        }
        bus_message->close_container();

        bus_message->open_container(SDBusMessage::M_MESSAGE_TYPE_ARRAY, "(iis)");
        for (const auto &cont_it : control_config) {
            bus_message->append_request_s(cont_it);
        }
        bus_message->close_container();

        std::shared_ptr<SDBusMessage> bus_reply = m_bus->call_method(bus_message);
        bus_reply->enter_container(SDBusMessage::M_MESSAGE_TYPE_STRUCT, "is");
        server_pid = bus_reply->read_integer();
        server_key = bus_reply->read_string();
        bus_reply->exit_container();
    }

    void ServiceProxyImp::platform_stop_batch(int server_pid)
    {
        m_bus->call_method("PlatformStopBatch", server_pid);
    }

    double ServiceProxyImp::platform_read_signal(const std::string &signal_name,
                                                 int domain,
                                                 int domain_idx)
    {
        std::shared_ptr<SDBusMessage> reply_message = m_bus->call_method("PlatformReadSignal",
                                                                         signal_name,
                                                                         domain,
                                                                         domain_idx);
        return reply_message->read_double();
    }

    void ServiceProxyImp::platform_write_control(const std::string &control_name,
                                                 int domain,
                                                 int domain_idx,
                                                 double setting)
    {
        (void)m_bus->call_method("PlatformWriteControl",
                                 control_name,
                                 domain,
                                 domain_idx,
                                 setting);
    }

    std::vector<std::string> ServiceProxyImp::read_string_array(
        std::shared_ptr<SDBusMessage> bus_message)
    {
        std::vector<std::string> result;
        bus_message->enter_container(SDBusMessage::M_MESSAGE_TYPE_ARRAY, "s");
        std::string str = bus_message->read_string();
        while (bus_message->was_success()) {
            result.push_back(str);
            str = bus_message->read_string();
        }
        bus_message->exit_container();
        return result;
    }
}
