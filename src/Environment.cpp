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

#ifdef __APPLE__
#define _DARWIN_C_SOURCE
const char *program_invocation_name = "geopm_profile";
#endif

#include "Environment.hpp"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "geopm_env.h"
#include "geopm_internal.h"
#include "Exception.hpp"
#include "Helper.hpp"

#include "config.h"

namespace geopm
{
    static Environment &test_environment(void)
    {
        static Environment instance;
        return instance;
    }

    const Environment &environment(void)
    {
        return test_environment();
    }

    Environment::Environment()
    {
        load();
    }

    void Environment::load()
    {
        m_report = "";
        m_comm = "MPIComm";
        m_policy = "";
        m_agent = "monitor";
        m_shmkey = "/geopm-shm-" + std::to_string(geteuid());
        m_trace = "";
        m_plugin_path = "";
        m_profile = "";
        m_max_fan_out = 16;
        m_pmpi_ctl = GEOPM_CTL_NONE;
        m_do_region_barrier = false;
        m_do_trace = false;
        m_do_profile = false;
        m_timeout = 30;
        m_debug_attach = -1;
        m_trace_signals = "";
        m_report_signals = "";

        std::string tmp_str;

        (void)get_env("GEOPM_REPORT", m_report);
        (void)get_env("GEOPM_COMM", m_comm);
        (void)get_env("GEOPM_POLICY", m_policy);
        (void)get_env("GEOPM_AGENT", m_agent);
        (void)get_env("GEOPM_SHMKEY", m_shmkey);
        if (m_shmkey[0] != '/') {
            m_shmkey = "/" + m_shmkey;
        }
        m_do_trace = get_env("GEOPM_TRACE", m_trace);
        (void)get_env("GEOPM_PLUGIN_PATH", m_plugin_path);
        m_do_region_barrier = get_env("GEOPM_REGION_BARRIER", tmp_str);
        (void)get_env("GEOPM_TIMEOUT", m_timeout);
        if (get_env("GEOPM_CTL", tmp_str)) {
            if (tmp_str == "process") {
                m_pmpi_ctl = GEOPM_CTL_PROCESS;
            }
            else if (tmp_str == "pthread") {
                m_pmpi_ctl = GEOPM_CTL_PTHREAD;
            }
            else {
                throw Exception("Environment::Environment(): " + tmp_str +
                                " is not a valid value for GEOPM_CTL see geopm(7).",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
        (void)get_env("GEOPM_DEBUG_ATTACH", m_debug_attach);
        m_do_profile = get_env("GEOPM_PROFILE", m_profile);
        (void)get_env("GEOPM_MAX_FAN_OUT", m_max_fan_out);
        if (m_report.length() ||
            m_do_trace ||
            m_pmpi_ctl != GEOPM_CTL_NONE) {
            m_do_profile = true;
        }
        if (m_do_profile && !m_profile.length()) {
            m_profile = program_invocation_name;
        }
        (void)get_env("GEOPM_TRACE_SIGNALS", m_trace_signals);
        (void)get_env("GEOPM_REPORT_SIGNALS", m_report_signals);
    }

    bool Environment::get_env(const char *name, std::string &env_string) const
    {
        bool result = false;
        char *check_string = getenv(name);
        if (check_string != NULL) {
            if (strlen(check_string) > NAME_MAX) {
                throw Exception("Environment::Environment(): Environment variable too long",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            env_string = check_string;
            result = true;
        }
        return result;
    }

    bool Environment::get_env(const char *name, int &value) const
    {
        bool result = false;
        std::string tmp_str("");
        char *end_ptr = NULL;

        if (get_env(name, tmp_str)) {
            value = strtol(tmp_str.c_str(), &end_ptr, 10);
            if (tmp_str.c_str() == end_ptr) {
                throw Exception("Environment::Environment(): Value could not be converted to an integer",
                                GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            result = true;
        }
        return result;
    }

    std::string Environment::report(void) const
    {
        return m_report;
    }

    std::string Environment::comm(void) const
    {
        return m_comm;
    }

    std::string Environment::policy(void) const
    {
        return m_policy;
    }

    std::string Environment::agent(void) const
    {
        return m_agent;
    }

    std::string Environment::shmkey(void) const
    {
        return m_shmkey;
    }

    std::string Environment::trace(void) const
    {
        return m_trace;
    }

    std::string Environment::profile(void) const
    {
        return m_profile;
    }

    std::string Environment::plugin_path(void) const
    {
        return m_plugin_path;
    }

    std::string Environment::trace_signals(void) const
    {
        return m_trace_signals;
    }

    std::string Environment::report_signals(void) const
    {
        return m_report_signals;
    }

    int Environment::max_fan_out(void) const
    {
        return m_max_fan_out;
    }

    int Environment::pmpi_ctl(void) const
    {
        return m_pmpi_ctl;
    }

    bool Environment::do_region_barrier(void) const
    {
        return m_do_region_barrier;
    }

    bool Environment::do_trace(void) const
    {
        return m_do_trace;
    }

    bool Environment::do_profile(void) const
    {
        return m_do_profile;
    }

    int Environment::timeout(void) const
    {
        return m_timeout;
    }

    int Environment::debug_attach(void) const
    {
        return m_debug_attach;
    }
}

extern "C"
{
    void geopm_env_load(void)
    {
        geopm::test_environment().load();
    }

    int geopm_env_pmpi_ctl(int *pmpi_ctl)
    {
        int err = 0;
        try {
            if (!pmpi_ctl) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *pmpi_ctl = geopm::environment().pmpi_ctl();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }

    int geopm_env_do_profile(int *do_profile)
    {
        int err = 0;
        try {
            if (!do_profile) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *do_profile = geopm::environment().do_profile();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }

    int geopm_env_debug_attach(int *debug_attach)
    {
        int err = 0;
        try {
            if (!debug_attach) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *debug_attach = geopm::environment().debug_attach();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }
}
