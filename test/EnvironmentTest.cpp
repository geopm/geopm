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
                              {"GEOPM_TRACE", vars.trace},
                              {"GEOPM_PROFILE", vars.profile},
                              {"GEOPM_REPORT", vars.report},
                              {"GEOPM_COMM", vars.comm},
                              {"GEOPM_POLICY", vars.policy},
                              {"GEOPM_AGENT", vars.agent},
                              {"GEOPM_SHMKEY", vars.shmkey},
                              {"GEOPM_FREQUENCY_MAP", vars.frequency_map},
                              {"GEOPM_TRACE_SIGNALS", vars.trace_signals},
                              {"GEOPM_REPORT_SIGNALS", vars.report_signals},
                              {"GEOPM_PLUGIN_PATH", vars.plugin_path},
                              {"GEOPM_CTL", vars.pmpi_ctl_str},
                              {"GEOPM_DEBUG_ATTACH", vars.debug_attach},
                              {"GEOPM_MAX_FAN_OUT", vars.max_fan_out},
                              {"GEOPM_TIMEOUT", vars.timeout},
                              {"GEOPM_REGION_BARRIER", vars.region_barrier},
                             };
    std::ofstream json_file_out(path, std::ifstream::out);
    json_file_out << root.dump();
    json_file_out.close();
}

void EnvironmentTest::SetUp()
{
    m_user.report = "report-test_value";
    m_user.policy = "policy-test_value";
    m_user.shmkey = "shmkey-test_value";
    m_user.trace = "trace-test_value";
    m_user.plugin_path = "plugin_path-test_value";
    m_user.profile = "profile-test_value";
    m_user.frequency_map = "hash:freq,hash:freq,hash:freq";
    m_user.region_barrier = std::to_string(true);
    m_user.timeout = std::to_string(30);
    m_user.debug_attach = std::to_string(-1);
    m_user.pmpi_ctl_str = "none";
    m_user.trace_signals = "test1,test2,test3";
    m_user.report_signals = "best1,best2,best3";

    TearDown();
}

void EnvironmentTest::TearDown()
{
    unsetenv("GEOPM_REPORT");
    unsetenv("GEOPM_POLICY");
    unsetenv("GEOPM_SHMKEY");
    unsetenv("GEOPM_TRACE");
    unsetenv("GEOPM_PLUGIN_PATH");
    unsetenv("GEOPM_REGION_BARRIER");
    unsetenv("GEOPM_ERROR_AFFINITY_IGNORE");
    unsetenv("GEOPM_TIMEOUT");
    unsetenv("GEOPM_CTL");
    unsetenv("GEOPM_DEBUG_ATTACH");
    unsetenv("GEOPM_PROFILE");
    unsetenv("GEOPM_COMM");
    unsetenv("GEOPM_AGENT");
    unsetenv("GEOPM_TRACE_SIGNALS");
    unsetenv("GEOPM_REPORT_SIGNALS");

    (void)unlink(M_DEFAULT_PATH.c_str());
    (void)unlink(M_OVERRIDE_PATH.c_str());
}

TEST_F(EnvironmentTest, construction0)
{
    int exp_pmpi_ctl = (int)GEOPM_CTL_NONE;
    struct environment_vars default_vars = m_user;
    struct environment_vars override_vars;//todo init list
    override_vars.report = "override-report-test_value";
    override_vars.policy = "override-policy-test_value";
    override_vars.shmkey = "override-shmkey-test_value";
    override_vars.trace = "override-trace-test_value";
    override_vars.plugin_path = "override-plugin_path-test_value";
    override_vars.profile = "override-profile-test_value";
    override_vars.frequency_map = "override:freq,hash:freq,hash:freq,hash:freq";
    override_vars.region_barrier = std::to_string(true);
    override_vars.timeout = std::to_string(15);
    override_vars.debug_attach = std::to_string(-1);
    override_vars.pmpi_ctl_str = "none";
    override_vars.trace_signals = "override,test1,test2,test3";
    override_vars.report_signals = "override,best1,best2,best3";
    for (size_t test_idx = 0; test_idx < 4; ++test_idx) {
        switch (test_idx) {
            case 0: // user only
                setenv("GEOPM_REPORT", m_user.report.c_str(), 1);
                setenv("GEOPM_POLICY", m_user.policy.c_str(), 1);
                setenv("GEOPM_SHMKEY", m_user.shmkey.c_str(), 1);
                setenv("GEOPM_TRACE", m_user.trace.c_str(), 1);
                setenv("GEOPM_PLUGIN_PATH", m_user.plugin_path.c_str(), 1);
                setenv("GEOPM_REGION_BARRIER", m_user.region_barrier.c_str(), 1);
                setenv("GEOPM_TIMEOUT", m_user.timeout.c_str(), 1);
                m_user.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                setenv("GEOPM_CTL", m_user.pmpi_ctl_str.c_str(), 1);
                setenv("GEOPM_DEBUG_ATTACH", m_user.debug_attach.c_str(), 1);
                setenv("GEOPM_PROFILE", m_user.profile.c_str(), 1);
                setenv("GEOPM_FREQUENCY_MAP", m_user.frequency_map.c_str(), 1);
                break;
            case 1: // default only
                default_vars.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(default_vars, M_DEFAULT_PATH);
                break;
            case 2: // override only
                override_vars.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(override_vars, M_OVERRIDE_PATH);
                break;
            case 3: // default and override
                default_vars.pmpi_ctl_str = std::string("process");
                exp_pmpi_ctl = (int)GEOPM_CTL_PROCESS;
                vars_to_json(default_vars, M_DEFAULT_PATH);
                override_vars.pmpi_ctl_str = std::string("pthread");
                exp_pmpi_ctl = (int)GEOPM_CTL_PTHREAD;
                vars_to_json(override_vars, M_OVERRIDE_PATH);
                break;
            case 4: // user, default and override
                //@todo
                break;
        }
        switch (test_idx) {
            case 0: // user only
                this->load("", "");
            case 1: // default only
                this->load(M_DEFAULT_PATH, "");
                break;
            case 2: // override only
                this->load("", M_OVERRIDE_PATH);
                break;
            case 3: // default and override
                this->load(M_DEFAULT_PATH, M_OVERRIDE_PATH);
                break;
            case 4: // user, default and override
                this->load(M_DEFAULT_PATH, M_OVERRIDE_PATH);
                break;
        }

        switch (test_idx) {
            case 0: // user only
                EXPECT_EQ(m_user.policy, this->policy());
                EXPECT_EQ("/" + m_user.shmkey, this->shmkey());
                EXPECT_EQ(m_user.trace, this->trace());
                EXPECT_EQ(m_user.trace.length() != 0, this->do_trace());
                EXPECT_EQ(m_user.plugin_path, this->plugin_path());
                EXPECT_EQ(m_user.report, this->report());
                EXPECT_EQ(m_user.profile, this->profile());
                EXPECT_EQ(m_user.profile.length() != 0, this->do_profile());
                EXPECT_EQ(m_user.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(m_user.region_barrier.length() != 0, this->do_region_barrier());
                EXPECT_EQ(m_user.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(m_user.debug_attach, std::to_string(this->debug_attach()));
                break;
            case 1: // default only
                EXPECT_EQ(default_vars.policy, this->policy());
                EXPECT_EQ("/" + default_vars.shmkey, this->shmkey());
                EXPECT_EQ(default_vars.trace, this->trace());
                EXPECT_EQ(default_vars.trace.length() != 0, this->do_trace());
                EXPECT_EQ(default_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(default_vars.report, this->report());
                EXPECT_EQ(default_vars.profile, this->profile());
                EXPECT_EQ(default_vars.profile.length() != 0, this->do_profile());
                EXPECT_EQ(default_vars.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(default_vars.region_barrier.length() != 0, this->do_region_barrier());
                EXPECT_EQ(default_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(default_vars.debug_attach, std::to_string(this->debug_attach()));
                break;
            case 2: // override only
                EXPECT_EQ(override_vars.policy, this->policy());
                EXPECT_EQ("/" + override_vars.shmkey, this->shmkey());
                EXPECT_EQ(override_vars.trace, this->trace());
                EXPECT_EQ(override_vars.trace.length() != 0, this->do_trace());
                EXPECT_EQ(override_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(override_vars.report, this->report());
                EXPECT_EQ(override_vars.profile, this->profile());
                EXPECT_EQ(override_vars.profile.length() != 0, this->do_profile());
                EXPECT_EQ(override_vars.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(override_vars.region_barrier.length() != 0, this->do_region_barrier());
                EXPECT_EQ(override_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(override_vars.debug_attach, std::to_string(this->debug_attach()));
                break;
            case 3: // default and override
                EXPECT_EQ(override_vars.policy, this->policy());
                EXPECT_EQ("/" + override_vars.shmkey, this->shmkey());
                EXPECT_EQ(override_vars.trace, this->trace());
                EXPECT_EQ(override_vars.trace.length() != 0, this->do_trace());
                EXPECT_EQ(override_vars.plugin_path, this->plugin_path());
                EXPECT_EQ(override_vars.report, this->report());
                EXPECT_EQ(override_vars.profile, this->profile());
                EXPECT_EQ(override_vars.profile.length() != 0, this->do_profile());
                EXPECT_EQ(override_vars.frequency_map, this->frequency_map());
                EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(override_vars.region_barrier.length() != 0, this->do_region_barrier());
                EXPECT_EQ(override_vars.timeout, std::to_string(this->timeout()));
                EXPECT_EQ(override_vars.debug_attach, std::to_string(this->debug_attach()));
                break;
            case 4: // user, default and override
                //@todo
                break;
        }
        TearDown();
    }
}

TEST_F(EnvironmentTest, construction1)
{
    setenv("GEOPM_REPORT", m_user.report.c_str(), 1);
    setenv("GEOPM_POLICY", m_user.policy.c_str(), 1);
    setenv("GEOPM_TRACE", m_user.trace.c_str(), 1);
    setenv("GEOPM_PLUGIN_PATH", m_user.plugin_path.c_str(), 1);
    //setenv("GEOPM_REGION_BARRIER", "", 1);
    setenv("GEOPM_TIMEOUT", m_user.timeout.c_str(), 1);
    m_user.pmpi_ctl_str = std::string("pthread");
    int exp_pmpi_ctl = (int)GEOPM_CTL_PTHREAD;
    setenv("GEOPM_CTL", m_user.pmpi_ctl_str.c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", m_user.debug_attach.c_str(), 1);
    //setenv("GEOPM_PROFILE", m_user.profile.c_str(), 1);
    setenv("GEOPM_TRACE_SIGNALS", m_user.trace_signals.c_str(), 1);
    setenv("GEOPM_REPORT_SIGNALS", m_user.report_signals.c_str(), 1);

    m_user.profile = program_invocation_name;

    this->load("", "");

    std::string default_shmkey("/geopm-shm-" + std::to_string(geteuid()));

    EXPECT_EQ(exp_pmpi_ctl, this->pmpi_ctl());
    EXPECT_EQ(1, this->do_profile());
    EXPECT_EQ(m_user.debug_attach, std::to_string(this->debug_attach()));
    EXPECT_EQ(m_user.policy, this->policy());
    EXPECT_EQ(default_shmkey, this->shmkey());
    EXPECT_EQ(m_user.trace, this->trace());
    EXPECT_EQ(m_user.trace.length() != 0, this->do_trace());
    EXPECT_EQ(m_user.plugin_path, this->plugin_path());
    EXPECT_EQ(m_user.report, this->report());
    EXPECT_EQ(m_user.profile, this->profile());
    EXPECT_EQ(m_user.profile.length() != 0, this->do_profile());
    EXPECT_EQ(0, this->do_region_barrier());
    EXPECT_EQ(m_user.timeout, std::to_string(this->timeout()));
    EXPECT_EQ(m_user.debug_attach, std::to_string(this->debug_attach()));
    EXPECT_EQ(m_user.trace_signals, this->trace_signals());
    EXPECT_EQ(m_user.report_signals, this->report_signals());
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
