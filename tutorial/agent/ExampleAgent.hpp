/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#ifndef EXAMPLEAGENT_HPP_INCLUDE
#define EXAMPLEAGENT_HPP_INCLUDE

#include <vector>

#include "geopm/Agent.hpp"
#include "geopm_time.h"

namespace geopm
{
    class IPlatformIO;
    class IPlatformTopo;
}

/// @brief Agent
class ExampleAgent : public geopm::Agent
{
    public:
        ExampleAgent();
        ExampleAgent(geopm::IPlatformIO &plat_io, geopm::IPlatformTopo &topo);
        virtual ~ExampleAgent() = default;
        void init(int level, const std::vector<int> &fan_in, bool is_level_root) override;
        std::vector<double> validate_policy(const std::vector<double> &in_policy) const override;
        bool descend(const std::vector<double> &in_policy,
                     std::vector<std::vector<double> >&out_policy) override;
        bool ascend(const std::vector<std::vector<double> > &in_sample,
                    std::vector<double> &out_sample) override;
        bool adjust_platform(const std::vector<double> &in_policy) override;
        bool sample_platform(std::vector<double> &out_sample) override;
        void wait(void) override;
        std::vector<std::pair<std::string, std::string> > report_header(void) const override;
        std::vector<std::pair<std::string, std::string> > report_node(void) const override;
        std::map<uint64_t, std::vector<std::pair<std::string, std::string> > > report_region(void) const override;
        std::vector<std::string> trace_names(void) const override;
        void trace_values(std::vector<double> &values) override;

        static std::string plugin_name(void);
        static std::unique_ptr<Agent> make_plugin(void);
        static std::vector<std::string> policy_names(void);
        static std::vector<std::string> sample_names(void);
    private:
        void load_trace_columns(void);

        // Policy indices; must match policy_names()
        enum m_policy_e {
            M_POLICY_LOW_THRESH,
            M_POLICY_HIGH_THRESH,
            M_NUM_POLICY
        };
        // Sample indices; must match sample_names()
        enum m_sample_e {
            M_SAMPLE_USER_PCT,
            M_SAMPLE_SYSTEM_PCT,
            M_SAMPLE_IDLE_PCT,
            M_NUM_SAMPLE
        };
        // Signals read in sample_platform()
        enum m_plat_signal_e {
            M_PLAT_SIGNAL_USER,
            M_PLAT_SIGNAL_SYSTEM,
            M_PLAT_SIGNAL_IDLE,
            M_PLAT_SIGNAL_NICE,
            M_NUM_PLAT_SIGNAL
        };
        // Controls written in adjust_platform()
        enum m_plat_control_e {
            M_PLAT_CONTROL_STDOUT,
            M_PLAT_CONTROL_STDERR,
            M_NUM_PLAT_CONTROL
        };
        // Values for trace
        enum m_trace_value_e {
            M_TRACE_VAL_USER_PCT,
            M_TRACE_VAL_SYSTEM_PCT,
            M_TRACE_VAL_IDLE_PCT,
            M_TRACE_VAL_SIGNAL_USER,
            M_TRACE_VAL_SIGNAL_SYSTEM,
            M_TRACE_VAL_SIGNAL_IDLE,
            M_TRACE_VAL_SIGNAL_NICE,

        };

        geopm::IPlatformIO &m_platform_io;
        geopm::IPlatformTopo &m_platform_topo;

        std::vector<int> m_signal_idx;
        std::vector<int> m_control_idx;
        std::vector<double> m_last_sample;
        std::vector<double> m_last_signal;

        geopm_time_s m_last_wait;
        const double M_WAIT_SEC;

        double m_min_idle;
        double m_max_idle;
};

#endif
