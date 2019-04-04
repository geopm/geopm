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
extern const char *program_invocation_name;
#endif

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <fstream>

#include "gtest/gtest.h"
#include "geopm_env.h"
#include "geopm_error.h"
#include "geopm_internal.h"
#include "Environment.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

#include "contrib/json11/json11.hpp"

using json11::Json;
using geopm::EnvironmentImp;

extern "C"
{
    void geopm_env_load(void);
}

class EnvironmentTest: public :: testing :: Test, protected EnvironmentImp
{
    protected:
        void SetUp();
        void TearDown();
        static void vars_to_json(struct environment_vars vars, const std::string &path);
        const std::string M_DEFAULT_PATH = "env_test_default.json";
        const std::string M_OVERRIDE_PATH = "env_test_override.json";
        struct environment_vars m_user;
};

void EnvironmentTest::vars_to_json(struct environment_vars vars, const std::string &path)
{
    Json root = Json::object {
                              {"GEOPM_REPORT", vars.report},
                              {"GEOPM_COMM", vars.comm},
                              {"GEOPM_POLICY", vars.policy},
                              {"GEOPM_AGENT", vars.agent},
                              {"GEOPM_SHMKEY", vars.shmkey},
                              {"GEOPM_TRACE", vars.trace},
                              {"GEOPM_PLUGIN_PATH", vars.plugin_path},
                              {"GEOPM_PROFILE", vars.profile},
                              {"GEOPM_FREQUENCY_MAP", vars.frequency_map},
                              {"GEOPM_CTL", vars.pmpi_ctl_str},
                              {"GEOPM_MAX_FAN_OUT", vars.max_fan_out},
                              {"GEOPM_TIMEOUT", vars.timeout},
                              {"GEOPM_DEBUG_ATTACH", vars.debug_attach},
                              {"GEOPM_TRACE_SIGNALS", vars.trace_signals},
                              {"GEOPM_REPORT_SIGNALS", vars.report_signals},
                              {"GEOPM_REGION_BARRIER", vars.region_barrier},
                             };
    std::ofstream json_file_out(path, std::ifstream::out);
    json_file_out << root.dump();
    json_file_out.close();
}

void EnvironmentTest::SetUp()
{
    m_user.report = "report-test_value";
    m_user.comm = "comm-test_value",
    m_user.policy = "policy-test_value";
    m_user.agent = "agent-test_value";
    m_user.shmkey = "shmkey-test_value";
    m_user.trace = "trace-test_value";
    m_user.plugin_path = "plugin_path-test_value";
    m_user.profile = "profile-test_value";
    m_user.frequency_map = "hash:freq,hash:freq,hash:freq";
    m_user.pmpi_ctl_str = "none";
    m_user.max_fan_out = std::to_string(16),
    m_user.timeout = std::to_string(30);
    m_user.debug_attach = std::to_string(1);
    m_user.trace_signals = "test1,test2,test3";
    m_user.report_signals = "best1,best2,best3";
    m_user.region_barrier = std::to_string(false);

    TearDown();
}

void EnvironmentTest::TearDown()
{
    unsetenv("GEOPM_REPORT");
    unsetenv("GEOPM_COMM");
    unsetenv("GEOPM_POLICY");
    unsetenv("GEOPM_AGENT");
    unsetenv("GEOPM_SHMKEY");
    unsetenv("GEOPM_TRACE");
    unsetenv("GEOPM_PLUGIN_PATH");
    unsetenv("GEOPM_PROFILE");
    unsetenv("GEOPM_FREQUENCY_MAP");
    unsetenv("GEOPM_CTL");
    unsetenv("GEOPM_MAX_FAN_OUT");
    unsetenv("GEOPM_TIMEOUT");
    unsetenv("GEOPM_DEBUG_ATTACH");
    unsetenv("GEOPM_TRACE_SIGNALS");
    unsetenv("GEOPM_REPORT_SIGNALS");
    unsetenv("GEOPM_REGION_BARRIER");

    (void)unlink(M_DEFAULT_PATH.c_str());
    (void)unlink(M_OVERRIDE_PATH.c_str());
}

TEST_F(EnvironmentTest, environment)
{
    int exp_pmpi_ctl = (int)GEOPM_CTL_NONE;
    struct environment_vars default_vars = {
                        .report = "default-report-test_value",
                        .comm = "default-comm-test_value",
                        .policy = "default-policy-test_value",
                        .agent = "default-agent-test_value",
                        .shmkey = "default-shmkey-test_value",
                        .trace = "default-trace-test_value",
                        .plugin_path = "default-plugin_path-test_value",
                        .profile = "default-profile-test_value",
                        .frequency_map = "default:freq,hash:freq,hash:freq,hash:freq",
                        .pmpi_ctl_str = "none",
                        .max_fan_out = std::to_string(16),
                        .timeout = std::to_string(0),
                        .debug_attach = std::to_string(-1),
                        .trace_signals = "default,test1,test2,test3",
                        .report_signals = "default,best1,best2,best3",
                        .region_barrier = std::to_string(true),
                    };
    struct environment_vars override_vars = {
                        .report = "override-report-test_value",
                        .comm = "override-comm-test_value",
                        .policy = "override-policy-test_value",
                        .agent = "override-agent-test_value",
                        .shmkey = "override-shmkey-test_value",
                        .trace = "override-trace-test_value",
                        .plugin_path = "override-plugin_path-test_value",
                        .profile = "override-profile-test_value",
                        .frequency_map = "override:freq,hash:freq,hash:freq,hash:freq",
                        .pmpi_ctl_str = "none",
                        .max_fan_out = std::to_string(16),
                        .timeout = std::to_string(15),
                        .debug_attach = std::to_string(-1),
                        .trace_signals = "override,test1,test2,test3",
                        .report_signals = "override,best1,best2,best3",
                        .region_barrier = std::to_string(false),
                    };
    struct environment_vars sim_default_vars;
    sim_default_vars.max_fan_out = std::to_string(16);
    sim_default_vars.timeout = std::to_string(69);
    struct environment_vars sim_override_vars;
    sim_override_vars.comm = "override-comm-test_value";
    sim_override_vars.agent = "override-agent-test_value";
    sim_override_vars.plugin_path = "override-plugin_path-test_value";
    sim_override_vars.pmpi_ctl_str = "process";

    for (size_t test_idx = 0; test_idx < 6; ++test_idx) {
        switch (test_idx) {
            case 0: // internal defaults
                break;
            case 1: // user only
                setenv("GEOPM_REPORT", m_user.report.c_str(), 1);
                setenv("GEOPM_COMM", m_user.comm.c_str(), 1);
                setenv("GEOPM_POLICY", m_user.policy.c_str(), 1);
                setenv("GEOPM_AGENT", m_user.agent.c_str(), 1);
                setenv("GEOPM_SHMKEY", m_user.shmkey.c_str(), 1);
                setenv("GEOPM_TRACE", m_user.trace.c_str(), 1);
                setenv("GEOPM_PLUGIN_PATH", m_user.plugin_path.c_str(), 1);
                setenv("GEOPM_PROFILE", m_user.profile.c_str(), 1);
                setenv("GEOPM_FREQUENCY_MAP", m_user.frequency_map.c_str(), 1);
                m_user.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                setenv("GEOPM_CTL", m_user.pmpi_ctl_str.c_str(), 1);
                setenv("GEOPM_MAX_FAN_OUT", m_user.max_fan_out.c_str(), 1);
                setenv("GEOPM_TIMEOUT", m_user.timeout.c_str(), 1);
                setenv("GEOPM_DEBUG_ATTACH", m_user.debug_attach.c_str(), 1);
                setenv("GEOPM_TRACE_SIGNALS", m_user.trace_signals.c_str(), 1);
                setenv("GEOPM_REPORT_SIGNALS", m_user.report_signals.c_str(), 1);
                setenv("GEOPM_REGION_BARRIER", m_user.region_barrier.c_str(), 1);
                break;
            case 2: // default only
                default_vars.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(default_vars, M_DEFAULT_PATH);
                break;
            case 3: // override only
                override_vars.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(override_vars, M_OVERRIDE_PATH);
                break;
            case 4: // default and override
                default_vars.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(default_vars, M_DEFAULT_PATH);
                override_vars.pmpi_ctl_str = std::string("pthread");
                exp_pmpi_ctl = (int)GEOPM_CTL_PTHREAD;
                vars_to_json(override_vars, M_OVERRIDE_PATH);
                break;
            case 5: // user, default and override
                setenv("GEOPM_REPORT", m_user.report.c_str(), 1);
                setenv("GEOPM_COMM", m_user.comm.c_str(), 1);
                setenv("GEOPM_POLICY", m_user.policy.c_str(), 1);
                setenv("GEOPM_AGENT", m_user.agent.c_str(), 1);
                setenv("GEOPM_AGENT", m_user.agent.c_str(), 1);
                setenv("GEOPM_TRACE", m_user.trace.c_str(), 1);
                setenv("GEOPM_PLUGIN_PATH", m_user.plugin_path.c_str(), 1);
                setenv("GEOPM_PROFILE", m_user.profile.c_str(), 1);
                setenv("GEOPM_FREQUENCY_MAP", m_user.frequency_map.c_str(), 1);
                setenv("GEOPM_CTL", m_user.pmpi_ctl_str.c_str(), 1);
                setenv("GEOPM_DEBUG_ATTACH", m_user.debug_attach.c_str(), 1);
                setenv("GEOPM_TRACE_SIGNALS", m_user.trace_signals.c_str(), 1);
                setenv("GEOPM_REPORT_SIGNALS", m_user.report_signals.c_str(), 1);
                setenv("GEOPM_REGION_BARRIER", m_user.region_barrier.c_str(), 1);
                // default
                vars_to_json(sim_default_vars, M_DEFAULT_PATH);
                // override
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(sim_override_vars, M_OVERRIDE_PATH);
                break;
        }
        switch (test_idx) {
            case 0: // internal defaults
            case 1: // user only
                this->load("", "");
                break;
            case 2: // default only
                this->load(M_DEFAULT_PATH, "");
                break;
            case 3: // override only
                this->load("", M_OVERRIDE_PATH);
                break;
            case 4: // default and override
                this->load(M_DEFAULT_PATH, M_OVERRIDE_PATH);
                break;
            case 5: // user, default and override
                this->load(M_DEFAULT_PATH, M_OVERRIDE_PATH);
                break;
        }

        switch (test_idx) {
            case 0: // internal defaults
                EXPECT_EQ("", this->report());
                EXPECT_EQ("MPIComm", this->comm());
                EXPECT_EQ("", this->policy());
                EXPECT_EQ("monitor", this->agent());
                EXPECT_EQ("/geopm-shm-" + std::to_string(geteuid()), this->shmkey());
                EXPECT_EQ("", this->trace());
                EXPECT_EQ(0, this->do_trace());
                EXPECT_EQ("", this->plugin_path());
                EXPECT_EQ(program_invocation_name, this->profile());
                EXPECT_EQ(0, this->do_profile());
                EXPECT_EQ("", this->frequency_map());
                EXPECT_THROW(this->pmpi_ctl(), geopm::Exception);   //!!!!!
                EXPECT_EQ(16, this->max_fan_out());
                EXPECT_EQ(30, this->timeout());
                EXPECT_EQ(-1, this->debug_attach());
                EXPECT_EQ("", this->trace_signals());
                EXPECT_EQ("", this->report_signals());
                EXPECT_EQ(false, this->do_region_barrier());
                break;
            case 1: // user only
                EXPECT_EQ(m_user.report, this->report());
                EXPECT_EQ(m_user.comm, this->comm());
                EXPECT_EQ(m_user.policy, this->policy());
                EXPECT_EQ(m_user.agent, this->agent());
                EXPECT_EQ("/" + m_user.shmkey, this->shmkey());
                EXPECT_EQ(m_user.trace, this->trace());
                EXPECT_EQ(m_user.do_trace(), this->do_trace());
                EXPECT_EQ(m_user.plugin_path, this->plugin_path());
                EXPECT_EQ(m_user.profile, this->profile());
                EXPECT_EQ(m_user.do_profile(), this->do_profile());
                EXPECT_EQ(m_user.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(m_user.max_fan_out, std::to_string(this->max_fan_out()));
                EXPECT_EQ(m_user.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(m_user.debug_attach, std::to_string(this->debug_attach()));
                EXPECT_EQ(m_user.trace_signals, this->trace_signals());
                EXPECT_EQ(m_user.report_signals, this->report_signals());
                EXPECT_EQ(m_user.do_region_barrier(), this->do_region_barrier());
                break;
            case 2: // default only
                EXPECT_EQ(default_vars.report, this->report());
                EXPECT_EQ(default_vars.comm, this->comm());
                EXPECT_EQ(default_vars.policy, this->policy());
                EXPECT_EQ(default_vars.agent, this->agent());
                EXPECT_EQ("/" + default_vars.shmkey, this->shmkey());
                EXPECT_EQ(default_vars.trace, this->trace());
                EXPECT_EQ(default_vars.do_trace(), this->do_trace());
                EXPECT_EQ(default_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(default_vars.profile, this->profile());
                EXPECT_EQ(default_vars.do_profile(), this->do_profile());
                EXPECT_EQ(default_vars.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(default_vars.max_fan_out, std::to_string(this->max_fan_out()));
                EXPECT_EQ(default_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(default_vars.debug_attach, std::to_string(this->debug_attach()));
                EXPECT_EQ(default_vars.trace_signals, this->trace_signals());
                EXPECT_EQ(default_vars.report_signals, this->report_signals());
                EXPECT_EQ(default_vars.do_region_barrier(), this->do_region_barrier());
                break;
            case 3: // override only
                EXPECT_EQ(override_vars.report, this->report());
                EXPECT_EQ(override_vars.comm, this->comm());
                EXPECT_EQ(override_vars.policy, this->policy());
                EXPECT_EQ(override_vars.agent, this->agent());
                EXPECT_EQ("/" + override_vars.shmkey, this->shmkey());
                EXPECT_EQ(override_vars.trace, this->trace());
                EXPECT_EQ(override_vars.do_trace(), this->do_trace());
                EXPECT_EQ(override_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(override_vars.profile, this->profile());
                EXPECT_EQ(override_vars.do_profile(), this->do_profile());
                EXPECT_EQ(override_vars.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(override_vars.max_fan_out, std::to_string(this->max_fan_out()));
                EXPECT_EQ(override_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(override_vars.debug_attach, std::to_string(this->debug_attach()));
                EXPECT_EQ(override_vars.trace_signals, this->trace_signals());
                EXPECT_EQ(override_vars.report_signals, this->report_signals());
                EXPECT_EQ(override_vars.do_region_barrier(), this->do_region_barrier());
                break;
            case 4: // default and override
                EXPECT_EQ(override_vars.report, this->report());
                EXPECT_EQ(override_vars.comm, this->comm());
                EXPECT_EQ(override_vars.policy, this->policy());
                EXPECT_EQ(override_vars.agent, this->agent());
                EXPECT_EQ("/" + override_vars.shmkey, this->shmkey());
                EXPECT_EQ(override_vars.trace, this->trace());
                EXPECT_EQ(override_vars.do_trace(), this->do_trace());
                EXPECT_EQ(override_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(override_vars.profile, this->profile());
                EXPECT_EQ(override_vars.do_profile(), this->do_profile());
                EXPECT_EQ(override_vars.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(override_vars.max_fan_out, std::to_string(this->max_fan_out()));
                EXPECT_EQ(override_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(override_vars.debug_attach, std::to_string(this->debug_attach()));
                EXPECT_EQ(override_vars.trace_signals, this->trace_signals());
                EXPECT_EQ(override_vars.report_signals, this->report_signals());
                EXPECT_EQ(override_vars.do_region_barrier(), this->do_region_barrier());
                break;
            case 5: // user, default and override
                EXPECT_EQ(m_user.report, this->report());
                EXPECT_EQ(sim_override_vars.comm, this->comm());
                EXPECT_EQ(m_user.policy, this->policy());
                EXPECT_EQ(sim_override_vars.agent, this->agent());
                EXPECT_EQ("/geopm-shm-" + std::to_string(geteuid()), this->shmkey());
                EXPECT_EQ(m_user.trace, this->trace());
                EXPECT_EQ(m_user.do_trace(), this->do_trace());
                EXPECT_EQ(sim_override_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(m_user.profile, this->profile());
                EXPECT_EQ(m_user.do_profile(), this->do_profile());
                EXPECT_EQ(m_user.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(sim_default_vars.max_fan_out, std::to_string(this->max_fan_out()));
                EXPECT_EQ(sim_default_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(m_user.debug_attach, std::to_string(this->debug_attach()));
                EXPECT_EQ(m_user.trace_signals, this->trace_signals());
                EXPECT_EQ(m_user.report_signals, this->report_signals());
                EXPECT_EQ(m_user.do_region_barrier(), this->do_region_barrier());
                break;
        }
        TearDown();
    }
}

TEST_F(EnvironmentTest, invalid_ctl)
{
    setenv("GEOPM_CTL", "program", 1);

    this->load("", "");

    EXPECT_THROW(this->pmpi_ctl(), geopm::Exception);
}

TEST_F(EnvironmentTest, c_apis)
{
    m_user.pmpi_ctl_str = std::string("process");
    setenv("GEOPM_CTL", m_user.pmpi_ctl_str.c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", m_user.debug_attach.c_str(), 1);
    setenv("GEOPM_PROFILE", m_user.profile.c_str(), 1);

    geopm_env_load();

    int test_pmpi_ctl = 0xdeadbeef;
    int test_do_profile = 0xdeadbeef;
    int test_debug_attach = 0xdeadbeef;
    (void)geopm_env_pmpi_ctl(&test_pmpi_ctl);
    (void)geopm_env_do_profile(&test_do_profile);
    (void)geopm_env_debug_attach(&test_debug_attach);
    EXPECT_EQ(GEOPM_CTL_PROCESS, test_pmpi_ctl);
}
