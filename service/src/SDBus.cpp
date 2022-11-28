/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "SDBus.hpp"

#include <sstream>
#include <systemd/sd-bus.h>

#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "SDBusMessage.hpp"

namespace geopm
{
    static void check_bus_error(const std::string &func_name,
                                int return_val,
                                const sd_bus_error *bus_error)
    {
        if (return_val < 0) {
            std::ostringstream error_message;
            error_message << "SDBus: Failed to call sd-bus function "
                          << func_name << "(), error:" << return_val;
            if (bus_error != nullptr) {
                error_message << " name: " << bus_error->name << ": "
                              << bus_error->message;
            }
            throw Exception(error_message.str(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::unique_ptr<SDBus> SDBus::make_unique(void)
    {
        return geopm::make_unique<SDBusImp>();
    }

    SDBusImp::SDBusImp()
        : m_dbus_destination("io.github.geopm")
        , m_dbus_path("/io/github/geopm")
        , m_dbus_interface("io.github.geopm")
        , m_dbus_timeout_usec(0)
    {
        int err = sd_bus_open_system(&m_bus);
        if (err < 0) {
            throw Exception("ServiceProxy: Failed to open system bus",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    SDBusImp::~SDBusImp()
    {
        sd_bus_close(m_bus);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::call_method(
        std::shared_ptr<SDBusMessage> message)
    {
        sd_bus_message *bus_reply;
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        int err = sd_bus_call(m_bus,
                              message->get_sd_ptr(),
                              m_dbus_timeout_usec,
                              &bus_error,
                              &bus_reply);
        check_bus_error("sd_bus_call", err, &bus_error);
        return SDBusMessage::make_unique(bus_reply);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::call_method(
        const std::string &member)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_reply = nullptr;
        int err = sd_bus_call_method(m_bus,
                                     m_dbus_destination,
                                     m_dbus_path,
                                     m_dbus_interface,
                                     member.c_str(),
                                     &bus_error,
                                     &bus_reply,
                                     "");
        check_bus_error("sd_bus_call_method", err, &bus_error);
        return SDBusMessage::make_unique(bus_reply);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::call_method(
        const std::string &member,
        const std::string &arg0,
        int arg1,
        int arg2)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_reply = nullptr;
        int err = sd_bus_call_method(m_bus,
                                     m_dbus_destination,
                                     m_dbus_path,
                                     m_dbus_interface,
                                     member.c_str(),
                                     &bus_error,
                                     &bus_reply,
                                     "sii",
                                     arg0.c_str(), arg1, arg2);
        check_bus_error("sd_bus_call_method", err, &bus_error);
        return SDBusMessage::make_unique(bus_reply);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::call_method(
        const std::string &member,
        const std::string &arg0,
        int arg1,
        int arg2,
        double arg3)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_reply = nullptr;
        int err = sd_bus_call_method(m_bus,
                                     m_dbus_destination,
                                     m_dbus_path,
                                     m_dbus_interface,
                                     member.c_str(),
                                     &bus_error,
                                     &bus_reply,
                                     "siid",
                                     arg0.c_str(), arg1, arg2, arg3);
        check_bus_error("sd_bus_call_method", err, &bus_error);
        return SDBusMessage::make_unique(bus_reply);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::call_method(
        const std::string &member,
        int arg0)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_reply = nullptr;
        int err = sd_bus_call_method(m_bus,
                                     m_dbus_destination,
                                     m_dbus_path,
                                     m_dbus_interface,
                                     member.c_str(),
                                     &bus_error,
                                     &bus_reply,
                                     "i",
                                     arg0);
        check_bus_error("sd_bus_call_method", err, &bus_error);
        return SDBusMessage::make_unique(bus_reply);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::call_method(
        const std::string &member,
        const std::string &arg0)
    {
        sd_bus_error bus_error = SD_BUS_ERROR_NULL;
        sd_bus_message *bus_reply = nullptr;
        int err = sd_bus_call_method(m_bus,
                                     m_dbus_destination,
                                     m_dbus_path,
                                     m_dbus_interface,
                                     member.c_str(),
                                     &bus_error,
                                     &bus_reply,
                                     "s",
                                     arg0.c_str());
        check_bus_error("sd_bus_call_method", err, &bus_error);
        return SDBusMessage::make_unique(bus_reply);
    }

    std::shared_ptr<SDBusMessage> SDBusImp::make_call_message(
        const std::string &member)
    {
        sd_bus_message *bus_message = nullptr;
        int err = sd_bus_message_new_method_call(m_bus,
                                                 &bus_message,
                                                 m_dbus_destination,
                                                 m_dbus_path,
                                                 m_dbus_interface,
                                                 member.c_str());
        check_bus_error("sd_bus_message_new_method_call", err, nullptr);
        return SDBusMessage::make_unique(bus_message);
    }
}
