/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <string>

#include "geopm_env.h"

namespace geopm
{
    class Environment
    {
        public:
            Environment();
            virtual ~Environment();
            const char *policy(void) const;
            const char *shmkey(void) const;
            const char *trace(void) const;
            const char *plugin_path(void) const;
            int do_pmpi_ctl(void) const;
            int do_region_barrier(void) const;
            int do_trace(void) const;
            int do_ignore_affinity() const;
        private:
            const std::string m_policy_env;
            const std::string m_shmkey_env;
            const std::string m_trace_env;
            const std::string m_plugin_path_env;
            int m_pmpi_ctl;
            const bool m_do_region_barrier;
            const bool m_do_trace;
            const bool m_do_ignore_affinity;
    };

    static const Environment &environment(void)
    {
        static const Environment instance;
        return instance;
    }

    Environment::Environment()
        : m_policy_env(getenv("GEOPM_POLICY") ? getenv("GEOPM_POLICY") : "")
        , m_shmkey_env(getenv("GEOPM_SHMKEY") ? getenv("GEOPM_SHMKEY") : "")
        , m_trace_env(getenv("GEOPM_TRACE") ? getenv("GEOPM_TRACE") : "")
        , m_plugin_path_env(getenv("GEOPM_PLUGIN_PATH") ? getenv("GEOPM_PLUGIN_PATH") : "")
        , m_do_region_barrier(getenv("GEOPM_REGION_BARRIER") != NULL ? true : false)
        , m_do_trace(m_trace_env.size() != 0 ? true : false)
        , m_do_ignore_affinity(getenv("GEOPM_ERROR_AFFININTY_IGNORE") != NULL ? true : false)
    {
        char *pmpi_ctl_env  = getenv("GEOPM_PMPI_CTL");
        if (pmpi_ctl_env && !strncmp(pmpi_ctl_env, "process", strlen("process") + 1))  {
            m_pmpi_ctl = GEOPM_PMPI_CTL_PROCESS;
        }
        else if (pmpi_ctl_env && !strncmp(pmpi_ctl_env, "pthread", strlen("pthread") + 1))  {
            m_pmpi_ctl = GEOPM_PMPI_CTL_PTHREAD;
        }
        else {
            m_pmpi_ctl = GEOPM_PMPI_CTL_NONE;
        }
    }

    Environment::~Environment()
    {

    }

    const char *Environment::policy(void) const
    {
        return m_policy_env.c_str();
    }

    const char *Environment::shmkey(void) const
    {
        return m_shmkey_env.c_str();
    }

    const char *Environment::trace(void) const
    {
        return m_trace_env.c_str();
    }

    const char *Environment::plugin_path(void) const
    {
        return m_plugin_path_env.c_str();
    }

    int Environment::do_pmpi_ctl(void) const
    {
        return m_pmpi_ctl;
    }

    int Environment::do_region_barrier(void) const
    {
        return m_do_region_barrier;
    }

    int Environment::do_trace(void) const
    {
        return m_do_trace;
    }

    int Environment::do_ignore_affinity() const
    {
        return m_do_ignore_affinity;
    }
}

extern "C"
{
    const char *geopm_env_policy(void)
    {
        return geopm::environment().policy();
    }

    const char *geopm_env_shmkey(void)
    {
        return geopm::environment().shmkey();
    }

    const char *geopm_env_trace(void)
    {
        return geopm::environment().trace();
    }

    const char *geopm_env_plugin_path(void)
    {
        return geopm::environment().plugin_path();
    }

    int geopm_env_do_pmpi_ctl(void)
    {
        return geopm::environment().do_pmpi_ctl();
    }

    int geopm_env_do_region_barrier(void)
    {
        return geopm::environment().do_region_barrier();
    }

    int geopm_env_do_trace(void)
    {
        return geopm::environment().do_trace();
    }

    int geopm_env_do_ignore_affinity(void)
    {
        return geopm::environment().do_ignore_affinity();
    }
}
