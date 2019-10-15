/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "Exception.hpp"

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mman.h>
#include <signal.h>
#include <limits.h>

#include "Environment.hpp"
#include "geopm_signal_handler.h"
#include "config.h"

namespace geopm
{
    class ErrorMessage
    {
        public:
            static ErrorMessage &get(void);
            void update(int error_value, const std::string &error_message);
            std::string message_last(int error_value);
            std::string message_fixed(int error_value);
        private:
            ErrorMessage();
            virtual ~ErrorMessage() = default;
            const std::map<int, std::string> M_VALUE_MESSAGE_MAP;
            int m_error_value;
            char m_error_message[NAME_MAX];
        public:
            ErrorMessage(const ErrorMessage &other) = delete;
            void operator=(const ErrorMessage &other) = delete;
    };
}

extern "C"
{
    void geopm_error_message(int error_value, char *error_message, size_t message_size)
    {
        std::string msg(geopm::ErrorMessage::get().message_last(error_value));
        strncpy(error_message, msg.c_str(), message_size - 1);
        if (msg.size() >= message_size) {
            error_message[message_size - 1] = '\0';
        }
    }

    static int geopm_env_shmkey(std::string &shmkey)
    {
        int err = 0;
        try {
            shmkey = geopm::environment().shmkey();
        }
        catch (...) {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    static std::string geopm_string_error(int error_value)
    {
        // Abstraction for GNU vs XSI definition of strerror_r()
        std::string result;
        char error_message[NAME_MAX];
#ifndef _GNU_SOURCE
        int undef = strerror_r(error_value, error_message, NAME_MAX);
        if (undef && undef != ERANGE) {
            result = "Unknown error: " +  std::to_string(err);
        }
        else {
            result = error_message;
        }
#else
        result = strerror_r(error_value, error_message, NAME_MAX);
#endif
        return result;
    }

    void geopm_error_destroy_shmem(void)
    {
        int err = 0;
        char err_msg[2 * NAME_MAX];
        DIR *did = opendir("/dev/shm");
        std::string key_base;
        err = geopm_env_shmkey(key_base);
        if (!err &&
            did &&
            strlen(key_base.c_str()) &&
            *(key_base.c_str()) == '/' &&
            strchr(key_base.c_str(), ' ') == NULL &&
            strchr(key_base.c_str() + 1, '/') == NULL) {

            struct dirent *entry;
            char shm_key[NAME_MAX];
            shm_key[0] = '/';
            shm_key[NAME_MAX - 1] = '\0';
            while ((entry = readdir(did))) {
                if (strstr(entry->d_name, key_base.c_str() + 1) == entry->d_name) {
                    strncpy(shm_key + 1, entry->d_name, NAME_MAX - 2);
                    err = shm_unlink(shm_key);
                    if (err) {
                        snprintf(err_msg, 2 * NAME_MAX, "Warning: <geopm> unable to unlink \"%s\"", shm_key);
                        perror(err_msg);
                    }
                }
            }
        }
    }
}

namespace geopm
{

    int exception_handler(std::exception_ptr eptr, bool do_print)
    {
        int err = errno ? errno : GEOPM_ERROR_RUNTIME;

        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        }
        catch (const std::exception &ex) {
            const geopm::SignalException *ex_geopm_signal = dynamic_cast<const geopm::SignalException *>(&ex);
            const geopm::Exception *ex_geopm = dynamic_cast<const geopm::Exception *>(&ex);
            const std::system_error *ex_sys = dynamic_cast<const std::system_error *>(&ex);

#ifdef GEOPM_DEBUG
            do_print = true;
#endif
            std::string message(ex.what());
            if (ex_geopm_signal) {
                err = ex_geopm_signal->err_value();
            }
            else if (ex_geopm) {
                err = ex_geopm->err_value();
            }
            else if (ex_sys) {
                err = ex_sys->code().value();
            }
            ErrorMessage::get().update(err, message);
            if (do_print) {
                std::cerr << "Error: " << message << std::endl;
            }
            if (ex_geopm_signal) {
                raise(ex_geopm_signal->sig_value());
            }
        }

        return err;
    }

    Exception::Exception(const std::string &what, int err, const char *file, int line)
        : std::runtime_error(ErrorMessage::get().message_fixed(err) + (
                                 what.size() != 0 ? (std::string(": ") + what) : std::string("")) + (
                                 file != NULL ? (std::string(": at geopm/") + std::string(file) +
                                 std::string(":") + std::to_string(line)) : std::string("")))
        , m_err(err ? err : GEOPM_ERROR_RUNTIME)
    {

    }

    Exception::Exception()
        : Exception("", GEOPM_ERROR_RUNTIME, NULL, 0)
    {

    }

    Exception::Exception(const Exception &other)
        : std::runtime_error(other.what())
        , m_err(other.m_err)
    {

    }

    Exception::Exception(int err)
        : Exception("", err, NULL, 0)
    {

    }

    Exception::Exception(const std::string &what, int err)
        : Exception(what, err, NULL, 0)
    {

    }

    Exception::Exception(int err, const char *file, int line)
        : Exception("", err, file, line)
    {

    }

    int Exception::err_value(void) const
    {
        return m_err;
    }


    SignalException::SignalException()
        : SignalException(0)
    {

    }

    SignalException::SignalException(const SignalException &other)
        : Exception(other)
        , m_sig(other.m_sig)
    {

    }

    SignalException::SignalException(int signum)
        : Exception(std::string("Signal ") + std::to_string(signum) + std::string(" raised"), errno ? errno : GEOPM_ERROR_RUNTIME)
        , m_sig(signum)
    {

    }

    SignalException::~SignalException()
    {

    }

    int SignalException::sig_value(void) const
    {
        return m_sig;
    }

    ErrorMessage::ErrorMessage()
        : M_VALUE_MESSAGE_MAP{
            {GEOPM_ERROR_RUNTIME, "Runtime error"},
            {GEOPM_ERROR_LOGIC, "Logic error"},
            {GEOPM_ERROR_INVALID, "Invalid argument"},
            {GEOPM_ERROR_FILE_PARSE, "Unable to parse input file"},
            {GEOPM_ERROR_LEVEL_RANGE, "Control hierarchy level is out of range"},
            {GEOPM_ERROR_NOT_IMPLEMENTED, "Feature not yet implemented"},
            {GEOPM_ERROR_PLATFORM_UNSUPPORTED, "Current platform not supported or unrecognized"},
            {GEOPM_ERROR_MSR_OPEN, "Could not open MSR device"},
            {GEOPM_ERROR_MSR_READ, "Could not read from MSR device"},
            {GEOPM_ERROR_MSR_WRITE, "Could not write to MSR device"},
            {GEOPM_ERROR_AGENT_UNSUPPORTED, "Specified Agent not supported or unrecognized"},
            {GEOPM_ERROR_AFFINITY, "MPI ranks are not affinitized to distinct CPUs"},
            {GEOPM_ERROR_NO_AGENT, "Requested agent is unavailable or invalid"},
            {GEOPM_ERROR_DATA_STORE, "Encountered a data store error"}}
        , m_error_value(0)
    {
        std::fill(m_error_message, m_error_message + NAME_MAX, '\0');
    }

    ErrorMessage &ErrorMessage::get(void)
    {
        static ErrorMessage instance;
        return instance;
    }

    void ErrorMessage::update(int error_value, const std::string &error_message)
    {
        size_t num_copy = error_message.size();
        // Never touch last byte of member so string is always null
        // terminated.
        if (num_copy > NAME_MAX - 2) {
            num_copy = NAME_MAX - 2;
        }
        std::copy(error_message.data(), error_message.data() + num_copy, m_error_message);
        m_error_message[num_copy] = '\0';
        m_error_value = error_value;
    }

    std::string ErrorMessage::message_last(int error_value)
    {
        if (error_value == m_error_value) {
            return m_error_message;
        }
        else {
            return message_fixed(error_value);
        }
    }

    std::string ErrorMessage::message_fixed(int err)
    {
        err = err ? err : GEOPM_ERROR_RUNTIME;
        std::string result("<geopm> ");
        auto it = M_VALUE_MESSAGE_MAP.find(err);
        if (it != M_VALUE_MESSAGE_MAP.end()) {
            result += it->second;
        }
        else {
            result += geopm_string_error(err);
        }
        return result;
    }
}
