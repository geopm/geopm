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
    class ErrorMessageLast
    {
        public:
            static ErrorMessageLast &get(void);
            void update(const std::string &msg);
            std::string message(void);
        private:
            ErrorMessageLast();
            virtual ~ErrorMessageLast() = default;
            char m_error_message[NAME_MAX];
        public:
            ErrorMessageLast(const ErrorMessageLast &other) = delete;
            void operator=(const ErrorMessageLast &other) = delete;
    };
}

extern "C"
{
    void geopm_error_message_last(char *msg_cstr, size_t size)
    {
       std::string msg(geopm::ErrorMessageLast::get().message());
       strncpy(msg_cstr, msg.c_str(), size - 1);
       if (msg.size() >= size) {
           msg_cstr[size - 1] = '\0';
       }
    }

    void geopm_error_message(int err, char *msg, size_t size)
    {
        switch (err) {
            case GEOPM_ERROR_RUNTIME:
                strncpy(msg, "<geopm> Runtime error", size);
                break;
            case GEOPM_ERROR_LOGIC:
                strncpy(msg, "<geopm> Logic error", size);
                break;
            case GEOPM_ERROR_INVALID:
                strncpy(msg, "<geopm> Invalid argument", size);
                break;
            case GEOPM_ERROR_FILE_PARSE:
                strncpy(msg, "<geopm> Unable to parse input file", size);
                break;
            case GEOPM_ERROR_LEVEL_RANGE:
                strncpy(msg, "<geopm> Control hierarchy level is out of range", size);
                break;
            case GEOPM_ERROR_NOT_IMPLEMENTED:
                strncpy(msg, "<geopm> Feature not yet implemented", size);
                break;
            case GEOPM_ERROR_PLATFORM_UNSUPPORTED:
                strncpy(msg, "<geopm> Current platform not supported or unrecognized", size);
                break;
            case GEOPM_ERROR_MSR_OPEN:
                strncpy(msg, "<geopm> Could not open MSR device", size);
                break;
            case GEOPM_ERROR_MSR_READ:
                strncpy(msg, "<geopm> Could not read from MSR device", size);
                break;
            case GEOPM_ERROR_MSR_WRITE:
                strncpy(msg, "<geopm> Could not write to MSR device", size);
                break;
            case GEOPM_ERROR_AGENT_UNSUPPORTED:
                strncpy(msg, "<geopm> Specified Agent not supported or unrecognized", size);
                break;
            case GEOPM_ERROR_AFFINITY:
                strncpy(msg, "<geopm> MPI ranks are not affinitized to distinct CPUs", size);
                break;
            case GEOPM_ERROR_NO_AGENT:
                strncpy(msg, "<geopm> Requested agent is unavailable or invalid", size);
                break;
            case GEOPM_ERROR_DATA_STORE:
                strncpy(msg, "<geopm> Encountered a data store error", size);
                break;
            default:
#ifndef _GNU_SOURCE
                int undef = strerror_r(err, msg, size);
                if (undef && undef != ERANGE) {
                    snprintf(msg, size, "<geopm> Unknown error: %i", err);
                }
#else
                strncpy(msg, strerror_r(err, msg, size), size);
#endif
                break;
        }
        if (size > 0) {
            msg[size-1] = '\0';
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

    static std::string error_message(int err);

    int exception_handler(std::exception_ptr eptr, bool do_print)
    {
        int err = GEOPM_ERROR_RUNTIME;
        try {
            if (eptr) {
                std::rethrow_exception(eptr);
            }
        }
        catch (const std::exception &ex) {
            const geopm::SignalException *ex_geopm_signal = dynamic_cast<const geopm::SignalException *>(&ex);
            const geopm::Exception *ex_geopm = dynamic_cast<const geopm::Exception *>(&ex);
            const std::system_error *ex_sys = dynamic_cast<const std::system_error *>(&ex);
            const std::runtime_error *ex_rt = dynamic_cast<const std::runtime_error *>(&ex);

#ifdef GEOPM_DEBUG
            do_print = true;
#endif
            std::string message;
            if (ex_geopm_signal) {
                message = ex_geopm_signal->what();
                err = ex_geopm->err_value();
            }
            else if (ex_geopm) {
                message = ex_geopm->what();
                err = ex_geopm->err_value();
            }
            else if (ex_sys) {
                message = ex_sys->what();
                err = ex_sys->code().value();
            }
            else if (ex_rt) {
                message = ex_rt->what();
                err = errno ? errno : GEOPM_ERROR_RUNTIME;
            }
            else {
                message = ex.what();
                err = errno ? errno : GEOPM_ERROR_RUNTIME;
            }
            ErrorMessageLast::get().update(message);
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
        : std::runtime_error(error_message(err) + (
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

    ErrorMessageLast &ErrorMessageLast::get(void)
    {
        static ErrorMessageLast instance;
        return instance;
    }

    void ErrorMessageLast::update(const std::string &msg)
    {
        size_t num_copy = msg.size();
        // Never touch last byte of member so string is always null
        // terminated.
        if (num_copy > NAME_MAX - 2) {
            num_copy = NAME_MAX - 2;
        }
        std::copy(msg.data(), msg.data() + num_copy, m_error_message);
        m_error_message[num_copy] = '\0';
    }

    std::string ErrorMessageLast::message(void)
    {
        return m_error_message;

    }

    ErrorMessageLast::ErrorMessageLast()
    {
        std::fill(m_error_message, m_error_message + NAME_MAX, '\0');
    }

    static std::string error_message(int err)
    {
        char tmp_msg[NAME_MAX];
        err = err ? err : GEOPM_ERROR_RUNTIME;
        geopm_error_message(err, tmp_msg, sizeof(tmp_msg));
        tmp_msg[NAME_MAX-1] = '\0';
        return tmp_msg;
    }
}
