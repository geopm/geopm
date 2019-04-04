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
            Environment() = default;
            virtual ~Environment() = default;
            virtual std::string report(void) const = 0;
            virtual std::string comm(void) const = 0;
            virtual std::string policy(void) const = 0;
            virtual std::string shmkey(void) const = 0;
            virtual std::string trace(void) const = 0;
            virtual std::string plugin_path(void) const = 0;
            virtual std::string profile(void) const = 0;
            virtual std::string frequency_map(void) const = 0;
            virtual std::string agent(void) const = 0;
            virtual std::string trace_signals(void) const = 0;
            virtual std::string report_signals(void) const = 0;
            virtual int max_fan_out(void) const = 0;
            virtual int pmpi_ctl(void) const = 0;
            virtual int do_region_barrier(void) const = 0;
            virtual int do_trace(void) const = 0;
            virtual int do_profile() const = 0;
            virtual int timeout(void) const = 0;
            virtual int debug_attach(void) const = 0;
    };

    class EnvironmentImp : public Environment
    {
        public:
            EnvironmentImp();
            virtual ~EnvironmentImp() = default;
            void load(void);
            std::string report(void) const override;
            std::string comm(void) const override;
            std::string policy(void) const override;
            std::string shmkey(void) const override;
            std::string trace(void) const override;
            std::string plugin_path(void) const override;
            std::string profile(void) const override;
            std::string frequency_map(void) const override;
            std::string agent(void) const override;
            std::string trace_signals(void) const override;
            std::string report_signals(void) const override;
            int max_fan_out(void) const override;
            int pmpi_ctl(void) const override;
            int do_region_barrier(void) const override;
            int do_trace(void) const override;
            int do_profile() const override;
            int timeout(void) const override;
            int debug_attach(void) const override;
        private:
            EnvironmentImp(const std::string &default_settings_path, const std::string &override_settings_path);
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
            std::string m_frequency_map;
            std::string m_pmpi_ctl_str;
            std::string m_max_fan_out;
            std::string m_timeout;
            std::string m_debug_attach;
            std::string m_trace_signals;
            std::string m_report_signals;
            std::string m_region_barrier;
            std::map<std::string, std::string&> m_exp_str_type;
    };


    const Environment &environment(void);
}
#endif
