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

#ifndef ENVIRONMENT_HPP_INCLUDE
#define ENVIRONMENT_HPP_INCLUDE

#include <map>
#include <string>

namespace geopm
{
    /// @brief Environment class encapsulates all functionality related to
    /// dealing with runtime environment variables.
    class Environment
    {
        public:
            Environment();
            virtual ~Environment() = default;
            void load(void);
            std::string report(void) const;
            std::string comm(void) const;
            std::string policy(void) const;
            std::string shmkey(void) const;
            std::string trace(void) const;
            std::string plugin_path(void) const;
            std::string profile(void) const;
            std::string agent(void) const;
            std::string trace_signals(void) const;
            std::string report_signals(void) const;
            int max_fan_out(void) const;
            int pmpi_ctl(void) const;
            int do_region_barrier(void) const;
            int do_trace(void) const;
            int do_profile() const;
            int timeout(void) const;
            int debug_attach(void) const;
        private:
            bool get_env(const char *name, std::string &env_string) const;
            bool get_env(const char *name, int &value) const;
            std::string m_report;
            std::string m_comm;
            std::string m_policy;
            std::string m_agent;
            std::string m_shmkey;
            std::string m_trace;
            std::string m_plugin_path;
            std::string m_profile;
            int m_max_fan_out;
            int m_pmpi_ctl;
            bool m_do_region_barrier;
            bool m_do_trace;
            bool m_do_profile;
            int m_timeout;
            int m_debug_attach;
            std::string m_trace_signals;
            std::string m_report_signals;
    };


    const Environment &environment(void);
}
#endif
