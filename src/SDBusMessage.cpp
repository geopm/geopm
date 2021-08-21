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

#include "SDBusMessage.hpp"
#include <cmath>
#include <sstream>
#include <systemd/sd-bus.h>

#include "Exception.hpp"
#include "Helper.hpp"

namespace geopm
{
    static void check_bus_error(const std::string &func_name,
                                int return_val)
    {
        if (return_val < 0) {
            std::ostringstream error_message;
            error_message << "SDBusMessage: Failed to call sd-bus function "
                          << func_name << "(), error:" << return_val;
            throw Exception(error_message.str(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    static void check_null_ptr(const std::string &method_name,
                               sd_bus_message *bus_message)
    {
        if (bus_message == nullptr) {
            std::ostringstream error_message;
            error_message << "SDBusMessage: Called method with NULL "
                          << "sd_bus_message pointer: SDBusMessageImp::"
                          << method_name << "()";
            throw Exception(error_message.str(),
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::unique_ptr<SDBusMessage> SDBusMessage::make_unique(sd_bus_message *bus_message)
    {
        return geopm::make_unique<SDBusMessageImp>(bus_message);
    }

    SDBusMessageImp::SDBusMessageImp()
        : SDBusMessageImp(nullptr)
    {

    }

    SDBusMessageImp::SDBusMessageImp(sd_bus_message *bus_message)
        : m_bus_message(bus_message)
        , m_was_success(false)
    {

    }

    sd_bus_message *SDBusMessageImp::get_sd_ptr(void)
    {
        return m_bus_message;
    }

    void SDBusMessageImp::enter_container(char type, const std::string &contents)
    {
        check_null_ptr(__func__, m_bus_message);
        switch (type) {
            case M_MESSAGE_TYPE_STRUCT:
                type = SD_BUS_TYPE_STRUCT;
                break;
            case M_MESSAGE_TYPE_ARRAY:
                type = SD_BUS_TYPE_ARRAY;
                break;
            default:
                throw Exception("Invalid type, not in SDBusMessage:m_message_type_e: " +
                                std::to_string(type),
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                break;
        }
        int ret = sd_bus_message_enter_container(m_bus_message,
                                                 type, contents.c_str());
        check_bus_error("sd_bus_message_enter_container", ret);
        m_was_success = (ret != 0);
    }

    void SDBusMessageImp::exit_container(void)
    {
        check_null_ptr(__func__, m_bus_message);
        int ret = sd_bus_message_exit_container(m_bus_message);
        check_bus_error("sd_bus_message_exit_container", ret);
        m_was_success = (ret != 0);
    }

    std::string SDBusMessageImp::read_string(void)
    {
        std::string result;
        const char *c_str = nullptr;
        int ret = sd_bus_message_read(m_bus_message, "s", &c_str);
        check_bus_error("sd_bus_message_read", ret);
        if (ret == 0) {
            m_was_success = false;
        }
        else {
            result = c_str;
            m_was_success = true;
        }
        return result;
    }

    double SDBusMessageImp::read_double(void)
    {
        check_null_ptr(__func__, m_bus_message);
        double result = NAN;
        int ret = sd_bus_message_read(m_bus_message, "d", &result);
        check_bus_error("sd_bus_message_read", ret);
        m_was_success = (ret != 0);
        return result;
    }

    int SDBusMessageImp::read_integer(void)
    {
        check_null_ptr(__func__, m_bus_message);
        int result = -1;
        int ret = sd_bus_message_read(m_bus_message, "i", &result);
        check_bus_error("sd_bus_message_read", ret);
        m_was_success = (ret != 0);
        return result;
    }

    void SDBusMessageImp::append_strings(const std::vector<std::string> &write_values)
    {
        check_null_ptr(__func__, m_bus_message);
        std::vector<const char *> write_values_cstr;
        for (const auto &wv : write_values) {
            write_values_cstr.push_back(wv.c_str());
        }
        write_values_cstr.push_back(nullptr);
        int ret = sd_bus_message_append_strv(m_bus_message,
                                             (char **)write_values_cstr.data());
        check_bus_error("sd_bus_message_append_strv", ret);
        m_was_success = true;
    }

    bool SDBusMessageImp::was_success(void)
    {
        return m_was_success;
    }
}
