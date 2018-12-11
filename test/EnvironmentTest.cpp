/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm_env.h"
#include "geopm_internal.h"
#include "Exception.hpp"

extern "C"
{
    void geopm_env_load(void);
}

class EnvironmentTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        std::string m_report;
        std::string m_policy;
        std::string m_shmkey;
        std::string m_trace;
        std::string m_plugin_path;
        std::string m_profile;
        std::string m_pmpi_ctl_str;
        int m_pmpi_ctl;
        bool m_do_region_barrier;
        bool m_do_trace;
        bool m_do_profile;
        int m_profile_timeout;
        int m_debug_attach;
};

void EnvironmentTest::SetUp()
{
    m_report = std::string("report-test_value");
    m_policy = std::string("policy-test_value");
    m_shmkey = std::string("shmkey-test_value");
    m_trace = std::string("trace-test_value");
    m_plugin_path = std::string("plugin_path-test_value");
    m_profile = std::string("profile-test_value");
    m_pmpi_ctl = GEOPM_CTL_NONE;
    m_do_region_barrier = false;
    m_do_trace = false;
    m_do_profile = false;
    m_profile_timeout = 30;
    m_debug_attach = -1;
    m_pmpi_ctl_str = std::string("none");

    unsetenv("GEOPM_REPORT");
    unsetenv("GEOPM_POLICY");
    unsetenv("GEOPM_SHMKEY");
    unsetenv("GEOPM_TRACE");
    unsetenv("GEOPM_PLUGIN_PATH");
    unsetenv("GEOPM_REGION_BARRIER");
    unsetenv("GEOPM_PROFILE_TIMEOUT");
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
    unsetenv("GEOPM_PROFILE_TIMEOUT");
    unsetenv("GEOPM_CTL");
    unsetenv("GEOPM_DEBUG_ATTACH");
    unsetenv("GEOPM_PROFILE");
    unsetenv("GEOPM_COMM");
    unsetenv("GEOPM_AGENT");
    unsetenv("GEOPM_TRACE_SIGNALS");
}

TEST_F(EnvironmentTest, construction0)
{
    setenv("GEOPM_REPORT", m_report.c_str(), 1);
    setenv("GEOPM_POLICY", m_policy.c_str(), 1);
    setenv("GEOPM_SHMKEY", m_shmkey.c_str(), 1);
    setenv("GEOPM_TRACE", m_trace.c_str(), 1);
    setenv("GEOPM_PLUGIN_PATH", m_plugin_path.c_str(), 1);
    setenv("GEOPM_REGION_BARRIER", "", 1);
    setenv("GEOPM_PROFILE_TIMEOUT", std::to_string(m_profile_timeout).c_str(), 1);
    m_pmpi_ctl_str = std::string("process");
    m_pmpi_ctl = GEOPM_CTL_PROCESS;
    setenv("GEOPM_CTL", m_pmpi_ctl_str.c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", std::to_string(m_debug_attach).c_str(), 1);
    setenv("GEOPM_PROFILE", m_profile.c_str(), 1);

    geopm_env_load();

    EXPECT_EQ(m_policy, std::string(geopm_env_policy()));
    EXPECT_EQ("/" + m_shmkey, std::string(geopm_env_shmkey()));
    EXPECT_EQ(m_trace, std::string(geopm_env_trace()));
    EXPECT_EQ(m_plugin_path, std::string(geopm_env_plugin_path()));
    EXPECT_EQ(m_report, std::string(geopm_env_report()));
    EXPECT_EQ(m_profile, std::string(geopm_env_profile()));
    EXPECT_EQ(m_pmpi_ctl, geopm_env_pmpi_ctl());
    EXPECT_EQ(1, geopm_env_do_region_barrier());
    EXPECT_EQ(1, geopm_env_do_trace());
    EXPECT_EQ(1, geopm_env_do_profile());
    EXPECT_EQ(m_profile_timeout, geopm_env_profile_timeout());
    EXPECT_EQ(m_debug_attach, geopm_env_debug_attach());
}

TEST_F(EnvironmentTest, construction1)
{
    setenv("GEOPM_REPORT", m_report.c_str(), 1);
    setenv("GEOPM_POLICY", m_policy.c_str(), 1);
    setenv("GEOPM_TRACE", m_trace.c_str(), 1);
    setenv("GEOPM_PLUGIN_PATH", m_plugin_path.c_str(), 1);
    //setenv("GEOPM_REGION_BARRIER", "", 1);
    setenv("GEOPM_PROFILE_TIMEOUT", std::to_string(m_profile_timeout).c_str(), 1);
    m_pmpi_ctl_str = std::string("pthread");
    m_pmpi_ctl = GEOPM_CTL_PTHREAD;
    setenv("GEOPM_CTL", m_pmpi_ctl_str.c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", std::to_string(m_debug_attach).c_str(), 1);
    //setenv("GEOPM_PROFILE", m_profile.c_str(), 1);
    setenv("GEOPM_TRACE_SIGNALS", "test1,test2,,test3", 0);

    m_profile = program_invocation_name;

    geopm_env_load();

    std::string default_shmkey("/geopm-shm-" + std::to_string(geteuid()));
    EXPECT_EQ(m_policy, std::string(geopm_env_policy()));
    EXPECT_EQ(default_shmkey, std::string(geopm_env_shmkey()));
    EXPECT_EQ(m_trace, std::string(geopm_env_trace()));
    EXPECT_EQ(m_plugin_path, std::string(geopm_env_plugin_path()));
    EXPECT_EQ(m_report, std::string(geopm_env_report()));
    EXPECT_EQ(m_profile, std::string(geopm_env_profile()));
    EXPECT_EQ(m_pmpi_ctl, geopm_env_pmpi_ctl());
    EXPECT_EQ(0, geopm_env_do_region_barrier());
    EXPECT_EQ(1, geopm_env_do_trace());
    EXPECT_EQ(1, geopm_env_do_profile());
    EXPECT_EQ(m_profile_timeout, geopm_env_profile_timeout());
    EXPECT_EQ(m_debug_attach, geopm_env_debug_attach());
    EXPECT_EQ(3, geopm_env_num_trace_signal());
    EXPECT_STREQ("test1", geopm_env_trace_signal(0));
    EXPECT_STREQ("test2", geopm_env_trace_signal(1));
    EXPECT_STREQ("test3", geopm_env_trace_signal(2));
}

TEST_F(EnvironmentTest, invalid_ctl)
{
    setenv("GEOPM_CTL", "program", 1);

    EXPECT_THROW(geopm_env_load(), geopm::Exception);
}
