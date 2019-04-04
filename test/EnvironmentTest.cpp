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
        void to_json(const std::string &path) const;
        void SetUp();
        void TearDown();
        const std::string M_DEFAULT_PATH = "env_test_default.json";
        const std::string M_OVERRIDE_PATH = "env_test_override.json";
        std::string m_report;
        std::string m_policy;
        std::string m_shmkey;
        std::string m_trace;
        std::string m_plugin_path;
        std::string m_profile;
        std::string m_frequency_map;
        std::string m_pmpi_ctl_str;
        std::string m_trace_signals;
        std::string m_report_signals;
        int m_pmpi_ctl;
        bool m_do_region_barrier;
        bool m_do_trace;
        bool m_do_profile;
        int m_timeout;
        int m_debug_attach;
};

void EnvironmentTest::to_json(const std::string &path) const
{
    // @todo arbitrary subset of all member, resolve
    Json root = Json::object {
                               {"GEOPM_REPORT", m_report},
                               {"GEOPM_POLICY", m_policy},
                               {"GEOPM_SHMKEY", m_shmkey},
                               {"GEOPM_TRACE", m_trace},
                               {"GEOPM_PLUGIN_PATH", m_plugin_path},
                               {"GEOPM_REGION_BARRIER", std::to_string(m_do_region_barrier)},
                               {"GEOPM_TIMEOUT", std::to_string(m_timeout)},
                               {"GEOPM_CTL", m_pmpi_ctl_str},
                               {"GEOPM_DEBUG_ATTACH", std::to_string(m_debug_attach)},
                               {"GEOPM_PROFILE", m_profile},
                               {"GEOPM_FREQUENCY_MAP", m_frequency_map},
                             };
    std::ofstream json_file_out(path, std::ifstream::out);
    json_file_out << root.dump();
    json_file_out.close();
}

void EnvironmentTest::SetUp()
{
    m_report = "report-test_value";
    m_policy = "policy-test_value";
    m_shmkey = "shmkey-test_value";
    m_trace = "trace-test_value";
    m_plugin_path = "plugin_path-test_value";
    m_profile = "profile-test_value";
    m_frequency_map = "hash:freq,hash:freq,hash:freq";
    m_pmpi_ctl = GEOPM_CTL_NONE;
    m_do_region_barrier = true;
    m_do_trace = true;
    m_do_profile = true;
    m_timeout = 30;
    m_debug_attach = -1;
    m_pmpi_ctl_str = "none";
    m_trace_signals = "test1,test2,test3";
    m_report_signals = "best1,best2,best3";

    unsetenv("GEOPM_REPORT");
    unsetenv("GEOPM_POLICY");
    unsetenv("GEOPM_SHMKEY");
    unsetenv("GEOPM_TRACE");
    unsetenv("GEOPM_PLUGIN_PATH");
    unsetenv("GEOPM_REGION_BARRIER");
    unsetenv("GEOPM_TIMEOUT");
    unsetenv("GEOPM_CTL");
    unsetenv("GEOPM_DEBUG_ATTACH");
    unsetenv("GEOPM_PROFILE");
    unsetenv("GEOPM_COMM");
    unsetenv("GEOPM_AGENT");
    unsetenv("GEOPM_TRACE_SIGNALS");
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
    (void)unlink(M_DEFAULT_PATH.c_str());
    (void)unlink(M_OVERRIDE_PATH.c_str());
}

TEST_F(EnvironmentTest, construction0)
{
    for (size_t test_idx = 0; test_idx < 4; ++test_idx) {
        switch (test_idx) {
            case 0: // user only
                setenv("GEOPM_REPORT", m_report.c_str(), 1);
                setenv("GEOPM_POLICY", m_policy.c_str(), 1);
                setenv("GEOPM_SHMKEY", m_shmkey.c_str(), 1);
                setenv("GEOPM_TRACE", m_trace.c_str(), 1);
                setenv("GEOPM_PLUGIN_PATH", m_plugin_path.c_str(), 1);
                setenv("GEOPM_REGION_BARRIER", std::to_string(m_do_region_barrier).c_str(), 1);
                setenv("GEOPM_TIMEOUT", std::to_string(m_timeout).c_str(), 1);
                m_pmpi_ctl_str = std::string("process");
                m_pmpi_ctl = GEOPM_CTL_PROCESS;
                setenv("GEOPM_CTL", m_pmpi_ctl_str.c_str(), 1);
                setenv("GEOPM_DEBUG_ATTACH", std::to_string(m_debug_attach).c_str(), 1);
                setenv("GEOPM_PROFILE", m_profile.c_str(), 1);
                setenv("GEOPM_FREQUENCY_MAP", m_frequency_map.c_str(), 1);
                break;
            case 1: // default only
                m_pmpi_ctl_str = std::string("process");
                m_pmpi_ctl = GEOPM_CTL_PROCESS;
                to_json(M_DEFAULT_PATH);
                break;
            case 2: // override only
                m_pmpi_ctl_str = std::string("process");
                m_pmpi_ctl = GEOPM_CTL_PROCESS;
                to_json(M_OVERRIDE_PATH);
                break;
            case 3: // default and override
                m_pmpi_ctl_str = std::string("process");
                m_pmpi_ctl = GEOPM_CTL_PROCESS;
                to_json(M_DEFAULT_PATH);
                m_pmpi_ctl_str = std::string("pthread");
                m_pmpi_ctl = GEOPM_CTL_PTHREAD;
                to_json(M_OVERRIDE_PATH);
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
            case 1: // default only
            case 2: // override only
            case 3: // default and override
                EXPECT_EQ(m_policy, this->policy());
                EXPECT_EQ("/" + m_shmkey, this->shmkey());
                EXPECT_EQ(m_trace, this->trace());
                EXPECT_EQ(m_plugin_path, this->plugin_path());
                EXPECT_EQ(m_report, this->report());
                EXPECT_EQ(m_profile, this->profile());
                EXPECT_EQ(m_frequency_map, this->frequency_map());
                EXPECT_EQ(m_pmpi_ctl, this->pmpi_ctl());
                EXPECT_EQ(m_do_region_barrier, this->do_region_barrier());
                EXPECT_EQ(m_do_trace, this->do_trace());
                EXPECT_EQ(m_do_profile, this->do_profile());
                EXPECT_EQ(m_timeout, this->timeout());
                EXPECT_EQ(m_debug_attach, this->debug_attach());
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
    setenv("GEOPM_REPORT", m_report.c_str(), 1);
    setenv("GEOPM_POLICY", m_policy.c_str(), 1);
    setenv("GEOPM_TRACE", m_trace.c_str(), 1);
    setenv("GEOPM_PLUGIN_PATH", m_plugin_path.c_str(), 1);
    //setenv("GEOPM_REGION_BARRIER", "", 1);
    m_do_region_barrier = false;
    setenv("GEOPM_TIMEOUT", std::to_string(m_timeout).c_str(), 1);
    m_pmpi_ctl_str = std::string("pthread");
    m_pmpi_ctl = GEOPM_CTL_PTHREAD;
    setenv("GEOPM_CTL", m_pmpi_ctl_str.c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", std::to_string(m_debug_attach).c_str(), 1);
    //setenv("GEOPM_PROFILE", m_profile.c_str(), 1);
    setenv("GEOPM_TRACE_SIGNALS", m_trace_signals.c_str(), 1);
    setenv("GEOPM_REPORT_SIGNALS", m_report_signals.c_str(), 1);

    m_profile = program_invocation_name;

    this->load("", "");

    std::string default_shmkey("/geopm-shm-" + std::to_string(geteuid()));

    EXPECT_EQ(m_pmpi_ctl, this->pmpi_ctl());
    EXPECT_EQ(1, this->do_profile());
    EXPECT_EQ(m_debug_attach, this->debug_attach());
    EXPECT_EQ(m_policy, this->policy());
    EXPECT_EQ(default_shmkey, this->shmkey());
    EXPECT_EQ(m_trace, this->trace());
    EXPECT_EQ(m_plugin_path, this->plugin_path());
    EXPECT_EQ(m_report, this->report());
    EXPECT_EQ(m_profile, this->profile());
    EXPECT_EQ(m_pmpi_ctl, this->pmpi_ctl());
    EXPECT_EQ(m_do_region_barrier, this->do_region_barrier());
    EXPECT_EQ(m_do_trace, this->do_trace());
    EXPECT_EQ(m_do_profile, this->do_profile());
    EXPECT_EQ(m_timeout, this->timeout());
    EXPECT_EQ(m_debug_attach, this->debug_attach());
    EXPECT_EQ(m_trace_signals, this->trace_signals());
    EXPECT_EQ(m_report_signals, this->report_signals());
}

TEST_F(EnvironmentTest, invalid_ctl)
{
    setenv("GEOPM_CTL", "program", 1);

    this->load("", "");

    EXPECT_THROW(this->pmpi_ctl(), geopm::Exception);
}

TEST_F(EnvironmentTest, c_apis)
{
    m_pmpi_ctl_str = std::string("process");
    setenv("GEOPM_CTL", m_pmpi_ctl_str.c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", std::to_string(m_debug_attach).c_str(), 1);
    setenv("GEOPM_PROFILE", m_profile.c_str(), 1);

    geopm_env_load();

    int test_pmpi_ctl = 0xdeadbeef;
    int test_do_profile = 0xdeadbeef;
    int test_debug_attach = 0xdeadbeef;
    (void)geopm_env_pmpi_ctl(&test_pmpi_ctl);
    (void)geopm_env_do_profile(&test_do_profile);
    (void)geopm_env_debug_attach(&test_debug_attach);
    EXPECT_EQ(GEOPM_CTL_PROCESS, test_pmpi_ctl);
}
