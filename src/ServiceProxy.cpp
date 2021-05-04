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

#include <systemd/sd-bus.h>
#include <sstream>

#include "Exception.hpp"
#include "Helper.hpp"
#include "geopm_internal.h"

namespace geopm
{

    std::unique_ptr<ServiceProxy> ServiceProxy::make_unique(void)
    {
        return geopm::make_unique<ServiceProxyImp>();
    }

    ServiceProxyImp::ServiceProxyImp()
    {
        int err = sd_bus_open_system(&m_bus);
        if (err < 0) {
            throw Exception("ServiceProxy: Failed to open system bus",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    ServiceProxyImp::~ServiceProxyImp()
    {
        sd_bus_close(m_bus);
    }

    void ServiceProxyImp::platform_get_user_access(std::vector<std::string> &signal_names,
                                                   std::vector<std::string> &control_names)
    {
        sd_bus_message *bus_message = call_method("PlatformGetUserAccess");
        int err = sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_STRUCT, "asas");
        if (err < 0) {
            throw Exception("ServiceProxy::platform_get_user_access(): Failed to enter (asas) container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        signal_names = read_string_array(bus_message);
        control_names = read_string_array(bus_message);
        err = sd_bus_message_exit_container(bus_message);
        if (err < 0) {
            throw Exception("ServiceProxy::platform_get_user_access(): Failed to exit \"(asas)\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::vector<signal_info_s> ServiceProxyImp::platform_get_signal_info(const std::vector<std::string> &signal_names)
    {
        std::vector<signal_info_s> result;
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_message = nullptr;
        std::vector<const char *> signal_names_cstr;
        for (const auto &name : signal_names) {
            signal_names_cstr.push_back(name.c_str());
        }
        int err = sd_bus_call_method(m_bus, "io.github.geopm",
                                     "/io/github/geopm",
                                     "io.github.geopm",
                                     "PlatformGetSignalInfo",
                                     &bus_error, &bus_message, "as",
                                     signal_names_cstr.data());
        if (err < 0) {
            std::ostringstream error_message;
            error_message << "ServiceProxy::platform_get_signal_info(): Failed to call sd-bus function sd_bus_call_method(), error:"
                          << err << " name: " << bus_error.name << ": " << bus_error.message;
            throw Exception(error_message.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_ARRAY, "(ssiiii)");
        if (err < 0) {
            throw Exception("ServiceProxy::platform_get_signal_info(): Failed to enter \"a(ssiiii)\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (bool is_done = false; !is_done;) {
            err = sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_STRUCT, "ssiiii");
            if (err < 0) {
                throw Exception("ServiceProxy::platform_get_signal_info(): Failed to enter \"(ssiiii)\" container",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (err == 0) {
                is_done = true;
            }
            else {
                const char *name = nullptr;
                err = sd_bus_message_read(bus_message, "s", &name);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to read name",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                const char *description = nullptr;
                err = sd_bus_message_read(bus_message, "s", &description);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to read description",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                int domain = -1;
                err = sd_bus_message_read(bus_message, "i", &domain);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to read domain",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                int aggregation = -1;
                err = sd_bus_message_read(bus_message, "i", &aggregation);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to read aggregation",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                int string_format = -1;
                err = sd_bus_message_read(bus_message, "i", &string_format);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to read string_format",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                int behavior = -1;
                err = sd_bus_message_read(bus_message, "i", &behavior);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to read behavior",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                err = sd_bus_message_exit_container(bus_message);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_signal_info(): Failed to exit \"(ssiiii)\" container",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                result.push_back({name, description, domain, aggregation, string_format, behavior});
            }
        }
        err = sd_bus_message_exit_container(bus_message);
        if (err < 0) {
            throw Exception("ServiceProxy::platform_get_signal_info(): Failed to exit \"a(ssiiii)\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    std::vector<control_info_s> ServiceProxyImp::platform_get_control_info(const std::vector<std::string> &control_names)
    {
        std::vector<control_info_s> result;
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_message = nullptr;
        std::vector<const char *> control_names_cstr;
        for (const auto &name : control_names) {
            control_names_cstr.push_back(name.c_str());
        }
        int err = sd_bus_call_method(m_bus, "io.github.geopm",
                                     "/io/github/geopm",
                                     "io.github.geopm",
                                     "PlatformGetControlInfo",
                                     &bus_error, &bus_message, "as",
                                     control_names_cstr.data());
        if (err < 0) {
            std::ostringstream error_message;
            error_message << "ServiceProxy::platform_get_control_info(): Failed to call sd-bus function sd_bus_call_method(), error:"
                          << err << " name: " << bus_error.name << ": " << bus_error.message;
            throw Exception(error_message.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_ARRAY, "(ssi)");
        if (err < 0) {
            throw Exception("ServiceProxy::platform_get_control_info(): Failed to enter \"a(ssi)\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (bool is_done = false; !is_done;) {
            err = sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_STRUCT, "ssi");
            if (err < 0) {
                throw Exception("ServiceProxy::platform_get_control_info(): Failed to enter \"(ssi)\" container",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (err == 0) {
                is_done = true;
            }
            else {
                const char *name = nullptr;
                err = sd_bus_message_read(bus_message, "s", &name);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_control_info(): Failed to read name",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                const char *description = nullptr;
                err = sd_bus_message_read(bus_message, "s", &description);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_control_info(): Failed to read description",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                int domain = -1;
                err = sd_bus_message_read(bus_message, "i", &domain);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_control_info(): Failed to read domain",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                err = sd_bus_message_exit_container(bus_message);
                if (err < 0) {
                    throw Exception("ServiceProxy::platform_get_control_info(): Failed to exit \"(ssiiii)\" container",
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                result.push_back({name, description, domain});
            }
        }
        err = sd_bus_message_exit_container(bus_message);
        if (err < 0) {
            throw Exception("ServiceProxy::platform_get_control_info(): Failed to exit \"a(ssiiii)\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }


    void ServiceProxyImp::platform_open_session(void)
    {
        (void)call_method("PlatformOpenSession");
    }

    void ServiceProxyImp::platform_close_session(void)
    {
        (void)call_method("PlatformCloseSession");
    }

    void ServiceProxyImp::platform_start_batch(const std::vector<struct geopm_request_s> &signal_config,
                                               const std::vector<struct geopm_request_s> &control_config,
                                               int &server_pid,
                                               std::string &server_key)
    {
        throw Exception("ServiceProxyImp::platform_start_batch()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void ServiceProxyImp::platform_stop_batch(int server_pid)
    {
        throw Exception("ServiceProxyImp::platform_stop_batch()",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double ServiceProxyImp::platform_read_signal(const std::string &signal_name,
                                                 int domain,
                                                 int domain_idx)
    {
        double result = NAN;
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_message = nullptr;
        int err = sd_bus_call_method(m_bus, "io.github.geopm",
                                     "/io/github/geopm",
                                     "io.github.geopm",
                                     "PlatformReadSignal",
                                     &bus_error, &bus_message, "sii",
                                     signal_name.c_str(),
                                     domain, domain_idx);
        if (err < 0) {
            std::ostringstream error_message;
            error_message << "ServiceProxy::platform_read_signal(): Failed to call sd-bus function sd_bus_call_method(), error:"
                          << err << " name: " << bus_error.name << ": " << bus_error.message;
            throw Exception(error_message.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = sd_bus_message_read(bus_message, "d", &result);
        if (err < 0) {
            throw Exception("ServiceProxy::platform_read_signal(): Failed to call sd-bus function sd_bus_meassage_read()",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    void ServiceProxyImp::platform_write_control(const std::string &control_name,
                                                 int domain,
                                                 int domain_idx,
                                                 double setting)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_message = nullptr;
        int err = sd_bus_call_method(m_bus, "io.github.geopm",
                                     "/io/github/geopm",
                                     "io.github.geopm",
                                     "PlatformWriteControl",
                                     &bus_error, &bus_message, "siid",
                                     control_name.c_str(),
                                     domain, domain_idx, setting);
        if (err < 0) {
            std::ostringstream error_message;
            error_message << "ServiceProxy::platform_write_control(): Failed to call sd-bus function sd_bus_call_method(), error:"
                          << err << " name: " << bus_error.name << ": " << bus_error.message;
            throw Exception(error_message.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    sd_bus_message *ServiceProxyImp::call_method(const std::string &method_name)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_message = nullptr;
        int err = sd_bus_call_method(m_bus, "io.github.geopm",
                                     "/io/github/geopm",
                                     "io.github.geopm",
                                     method_name.c_str(),
                                     &bus_error, &bus_message, "");
        if (err < 0) {
            std::ostringstream error_message;
            error_message << "ServiceProxyImp:call_method(): Failed to call sd-bus function sd_bus_call_method, error:"
                          << err << ": " << bus_error.name << ": " << bus_error.message;
            throw Exception(error_message.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return bus_message;
    }

    std::vector<std::string> ServiceProxyImp::read_string_array(sd_bus_message *bus_message)
    {
        std::vector<std::string> result;
        int err = sd_bus_message_enter_container(bus_message, SD_BUS_TYPE_ARRAY, "s");
        if (err < 0) {
            throw Exception("ServiceProxyImp::read_string_array(): Failed to enter \"as\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        for (bool is_done = false; !is_done;) {
            const char *c_str = nullptr;
            int err = sd_bus_message_read(bus_message, "s", &c_str);
            if (err < 0) {
                throw Exception("ServiceProxyImp::read_string_array(): Failed to read string",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (err == 0) {
                is_done = true;
            }
            else {
                result.push_back(c_str);
            }
        }
        err = sd_bus_message_exit_container(bus_message);
        if (err < 0) {
            throw Exception("ServiceProxyImp::read_string_array(): Failed to enter \"as\" container",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }
}
