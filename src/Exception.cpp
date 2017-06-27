/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mman.h>
#include <signal.h>


#include "Exception.hpp"
#include "geopm_env.h"
#include "geopm_signal_handler.h"
#include "config.h"

#ifndef NAME_MAX
#define NAME_MAX 1024
#endif

extern "C"
{
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
            case GEOPM_ERROR_POLICY_NULL:
                strncpy(msg, "<geopm> The geopm_policy_c pointer is NULL, use geopm_policy_create()", size);
                break;
            case GEOPM_ERROR_FILE_PARSE:
                strncpy(msg, "<geopm> Unable to parse input file", size);
                break;
            case GEOPM_ERROR_LEVEL_RANGE:
                strncpy(msg, "<geopm> Control hierarchy level is out of range", size);
                break;
            case GEOPM_ERROR_CTL_COMM:
                strncpy(msg, "<geopm> Communication error in control hierarchy", size);
                break;
            case GEOPM_ERROR_SAMPLE_INCOMPLETE:
                strncpy(msg, "<geopm> All children have not sent all samples", size);
                break;
            case GEOPM_ERROR_POLICY_UNKNOWN:
                strncpy(msg, "<geopm> No policy has been set", size);
                break;
            case GEOPM_ERROR_NOT_IMPLEMENTED:
                strncpy(msg, "<geopm> Feature not yet implemented", size);
                break;
            case GEOPM_ERROR_NOT_TESTED:
                strncpy(msg, "<geopm> Feature not yet tested", size);
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
            case GEOPM_ERROR_OPENMP_UNSUPPORTED:
                strncpy(msg, "<geopm> Not compiled with support for OpenMP", size);
                break;
            case GEOPM_ERROR_PROF_NULL:
                strncpy(msg, "<geopm> The geopm_prof_c pointer is NULL, use geopm_prof_create()", size);
                break;
            case GEOPM_ERROR_DECIDER_UNSUPPORTED:
                strncpy(msg, "<geopm> Specified Decider not supported or unrecognized", size);
                break;
            case GEOPM_ERROR_FACTORY_NULL:
                strncpy(msg, "<geopm> The geopm_factory_c pointer is NULL, pass in a valid factory object", size);
                break;
            case GEOPM_ERROR_SHUTDOWN:
                strncpy(msg, "<geopm> Shutdown policy has been signaled", size);
                break;
            case GEOPM_ERROR_TOO_MANY_COLLISIONS:
                strncpy(msg, "<geopm> Too many collisions when inserting into hash table", size);
                break;
            case GEOPM_ERROR_AFFINITY:
                strncpy(msg, "<geopm> MPI ranks are not affitinized to distinct CPUs", size);
                break;
            case GEOPM_ERROR_ENVIRONMENT:
                strncpy(msg, "<geopm> Unset or invalid environment variable", size);
                break;
            case GEOPM_ERROR_COMM_UNSUPPORTED:
                strncpy(msg, "<geopm> Communication implementation not supported", size);
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

    void geopm_error_destroy_shmem(void)
    {
        int err = 0;
        char err_msg[NAME_MAX];
        DIR *did = opendir("/dev/shm");
        if (did &&
            strlen(geopm_env_shmkey()) &&
            *(geopm_env_shmkey()) == '/' &&
            strchr(geopm_env_shmkey(), ' ') == NULL &&
            strchr(geopm_env_shmkey() + 1, '/') == NULL) {

            struct dirent *entry;
            char shm_key[NAME_MAX];
            shm_key[0] = '/';
            shm_key[NAME_MAX - 1] = '\0';
            while ((entry = readdir(did))) {
                if (strstr(entry->d_name, geopm_env_shmkey() + 1) == entry->d_name) {
                    strncpy(shm_key + 1, entry->d_name, NAME_MAX - 2);
                    err = shm_unlink(shm_key);
                    if (err) {
                        snprintf(err_msg, NAME_MAX, "Warning: <geopm> unable to unlink \"%s\"", shm_key);
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

    int exception_handler(std::exception_ptr eptr)
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

            if (ex_geopm_signal) {
#ifdef GEOPM_DEBUG
                std::cerr << "Error: " << ex_geopm_signal->what() << std::endl;
#endif
                err = ex_geopm->err_value();
                raise(ex_geopm_signal->sig_value());
            }
            else if (ex_geopm) {
#ifdef GEOPM_DEBUG
                std::cerr << "Error: " << ex_geopm->what() << std::endl;
#endif
                err = ex_geopm->err_value();
            }
            else if (ex_sys) {
#ifdef GEOPM_DEBUG
                std::cerr << "Error: " << ex_sys->what() << std::endl;
#endif
                err = ex_sys->code().value();
            }
            else if (ex_rt) {
#ifdef GEOPM_DEBUG
                std::cerr << "Error: " << ex_rt->what() << std::endl;
#endif
                err = errno ? errno : GEOPM_ERROR_RUNTIME;
            }
            else {
#ifdef GEOPM_DEBUG
                std::cerr << "Error: " << ex.what() << std::endl;
#endif
                err = errno ? errno : GEOPM_ERROR_RUNTIME;
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

    Exception::~Exception()
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

    static std::string error_message(int err)
    {
        char tmp_msg[NAME_MAX];
        err = err ? err : GEOPM_ERROR_RUNTIME;
        geopm_error_message(err, tmp_msg, sizeof(tmp_msg));
        tmp_msg[NAME_MAX-1] = '\0';
        return tmp_msg;
    }
}
