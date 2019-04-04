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
        static void vars_to_json(std::map<std::string, std::string> vars, const std::string &path);
        void expect_vars(std::map<std::string, std::string> exp_vars) const;
        const std::string M_DEFAULT_PATH = "env_test_default.json";
        const std::string M_OVERRIDE_PATH = "env_test_override.json";
        std::map<std::string, std::string> m_user;
        std::map<std::string, int> m_pmpi_ctl_map;
};

void EnvironmentTest::vars_to_json(std::map<std::string, std::string> vars, const std::string &path)
{
    std::ofstream json_file_out(path, std::ifstream::out);
    json_file_out << "{";
    bool seen_first = false;
    for (const auto &kv : vars) {
        if (seen_first) {
            json_file_out << ",";
        }
        else {
            seen_first = true;
        }
        json_file_out << "\"" << kv.first << "\":";
        json_file_out << "\"" << kv.second << "\"";
    }
    json_file_out << "}";
    json_file_out.close();
}

void EnvironmentTest::expect_vars(std::map<std::string, std::string> exp_vars) const
{
    EXPECT_EQ(exp_vars["GEOPM_REPORT"], this->report());
    EXPECT_EQ(exp_vars["GEOPM_COMM"], this->comm());
    EXPECT_EQ(exp_vars["GEOPM_POLICY"], this->policy());
    EXPECT_EQ(exp_vars["GEOPM_AGENT"], this->agent());
    EXPECT_EQ(exp_vars["GEOPM_SHMKEY"], this->shmkey());
    EXPECT_EQ(exp_vars["GEOPM_TRACE"], this->trace());
    //EXPECT_EQ(exp_vars.do_trace(), this->do_trace());
    EXPECT_EQ(exp_vars["GEOPM_PLUGIN_PATH"], this->plugin_path());
    EXPECT_EQ(exp_vars["GEOPM_PROFILE"], this->profile());
    //EXPECT_EQ(exp_vars.do_profile(), this->do_profile());
    EXPECT_EQ(exp_vars["GEOPM_FREQUENCY_MAP"], this->frequency_map());
    auto it = m_pmpi_ctl_map.find(exp_vars["GEOPM_CTL"]);
    if (it == m_pmpi_ctl_map.end()) {
        EXPECT_THROW(this->pmpi_ctl(), geopm::Exception);
    }
    else {
        EXPECT_EQ(it->second, this->pmpi_ctl());
    }
    EXPECT_EQ(exp_vars["GEOPM_MAX_FAN_OUT"], std::to_string(this->max_fan_out()));
    EXPECT_EQ(exp_vars["GEOPM_TIMEOUT"], std::to_string(this->timeout()));
    EXPECT_EQ(exp_vars["GEOPM_DEBUG_ATTACH"], std::to_string(this->debug_attach()));
    EXPECT_EQ(exp_vars["GEOPM_TRACE_SIGNALS"], this->trace_signals());
    EXPECT_EQ(exp_vars["GEOPM_REPORT_SIGNALS"], this->report_signals());
    EXPECT_EQ(exp_vars.find("GEOPM_REGION_BARRIER") != exp_vars.end(), this->do_region_barrier());
}

void EnvironmentTest::SetUp()
{
    m_user = {
              {"GEOPM_REPORT", "report-test_value"},
              {"GEOPM_COMM", "comm-test_value"},
              {"GEOPM_POLICY", "policy-test_value"},
              {"GEOPM_AGENT", "agent-test_value"},
              {"GEOPM_TRACE", "trace-test_value"},
              {"GEOPM_PLUGIN_PATH", "plugin_path-test_value"},
              {"GEOPM_PROFILE", "profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "none"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_DEBUG_ATTACH", "1"},
              {"GEOPM_TRACE_SIGNALS", "test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "best1,best2,best3"},
              {"GEOPM_REGION_BARRIER", std::to_string(false)},
             };

    m_pmpi_ctl_map["process"] = (int)GEOPM_CTL_PROCESS;
    m_pmpi_ctl_map["pthread"] = (int)GEOPM_CTL_PTHREAD;

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

TEST_F(EnvironmentTest, internal_defaults)
{
    // @todo program_invocation_name test
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_CTL", "none"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };

    this->load("", "");
    std::map<std::string, std::string> exp_vars = internal_default_vars;
    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_CTL", "none"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    this->load("", "");
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_SHMKEY"] = internal_default_vars["GEOPM_SHMKEY"];
    exp_vars["GEOPM_TIMEOUT"] = internal_default_vars["GEOPM_TIMEOUT"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, default_only)
{
    std::map<std::string, std::string> default_vars = {
              {"GEOPM_REPORT", "default-report-test_value"},
              {"GEOPM_COMM", "default-comm-test_value"},
              {"GEOPM_POLICY", "default-policy-test_value"},
              {"GEOPM_AGENT", "default-agent-test_value"},
              {"GEOPM_SHMKEY", "default-shmkey-test_value"},
              {"GEOPM_TRACE", "default-trace-test_value"},
              {"GEOPM_PLUGIN_PATH", "default-plugin_path-test_value"},
              {"GEOPM_PROFILE", "default-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "default-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "pthread"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "0"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "default-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "default-best1,best2,best3"},
              {"GEOPM_REGION_BARRIER", std::to_string(false)},
             };
    vars_to_json(default_vars, M_DEFAULT_PATH);

    this->load(M_DEFAULT_PATH, "");
    std::map<std::string, std::string> exp_vars = default_vars;
    exp_vars["GEOPM_SHMKEY"] = "/" + exp_vars["GEOPM_SHMKEY"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, override_only)
{
    std::map<std::string, std::string> override_vars = {
              {"GEOPM_REPORT", "override-report-test_value"},
              {"GEOPM_COMM", "override-comm-test_value"},
              {"GEOPM_POLICY", "override-policy-test_value"},
              {"GEOPM_AGENT", "override-agent-test_value"},
              {"GEOPM_SHMKEY", "/override-shmkey-test_value"},
              {"GEOPM_TRACE", "override-trace-test_value"},
              {"GEOPM_PLUGIN_PATH", "override-plugin_path-test_value"},
              {"GEOPM_PROFILE", "override-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "override-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "process"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "15"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "override-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "override-best1,best2,best3"},
              {"GEOPM_REGION_BARRIER", std::to_string(false)},
             };
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    this->load("", M_OVERRIDE_PATH);
    std::map<std::string, std::string> exp_vars = override_vars;

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, default_and_override)
{
    std::map<std::string, std::string> default_vars = {
              {"GEOPM_REPORT", "default-report-test_value"},
              {"GEOPM_COMM", "default-comm-test_value"},
              {"GEOPM_POLICY", "default-policy-test_value"},
              {"GEOPM_AGENT", "default-agent-test_value"},
              {"GEOPM_SHMKEY", "default-shmkey-test_value"},
              {"GEOPM_TRACE", "default-trace-test_value"},
              {"GEOPM_PLUGIN_PATH", "default-plugin_path-test_value"},
              {"GEOPM_PROFILE", "default-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "default-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "pthread"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "0"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "default-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "default-best1,best2,best3"},
              {"GEOPM_REGION_BARRIER", std::to_string(false)},
             };
    std::map<std::string, std::string> override_vars = {
              {"GEOPM_REPORT", "override-report-test_value"},
              {"GEOPM_COMM", "override-comm-test_value"},
              {"GEOPM_POLICY", "override-policy-test_value"},
              {"GEOPM_AGENT", "override-agent-test_value"},
              {"GEOPM_SHMKEY", "/override-shmkey-test_value"},
              {"GEOPM_TRACE", "override-trace-test_value"},
              {"GEOPM_PLUGIN_PATH", "override-plugin_path-test_value"},
              {"GEOPM_PROFILE", "override-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "override-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "process"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "15"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "override-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "override-best1,best2,best3"},
              {"GEOPM_REGION_BARRIER", std::to_string(false)},
             };

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    this->load(M_DEFAULT_PATH, M_OVERRIDE_PATH);
    std::map<std::string, std::string> exp_vars = override_vars;
    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_default_and_override)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_CTL", "none"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    std::map<std::string, std::string> default_vars = {
                {"GEOPM_MAX_FAN_OUT", "16"},
                {"GEOPM_TIMEOUT", "69"},
            };
    std::map<std::string, std::string> override_vars = {
              {"GEOPM_COMM", "override-comm-test_value"},
              {"GEOPM_AGENT", "override-agent-test_value"},
              {"GEOPM_PLUGIN_PATH", "override-plugin_path-test_value"},
              {"GEOPM_CTL", "process"},
    };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }
    // default
    vars_to_json(default_vars, M_DEFAULT_PATH);
    // override
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    this->load(M_DEFAULT_PATH, M_OVERRIDE_PATH);
    std::map<std::string, std::string> exp_vars = {
        {"GEOPM_REPORT", m_user["GEOPM_REPORT"]},
        {"GEOPM_COMM", override_vars["GEOPM_COMM"]},
        {"GEOPM_POLICY", m_user["GEOPM_POLICY"]},
        {"GEOPM_AGENT", override_vars["GEOPM_AGENT"]},
        {"GEOPM_SHMKEY", internal_default_vars["GEOPM_SHMKEY"]},
        {"GEOPM_TRACE", m_user["GEOPM_TRACE"]},
        {"GEOPM_PLUGIN_PATH", override_vars["GEOPM_PLUGIN_PATH"]},
        {"GEOPM_PROFILE", m_user["GEOPM_PROFILE"]},
        {"GEOPM_FREQUENCY_MAP", m_user["GEOPM_FREQUENCY_MAP"]},
        {"GEOPM_CTL", override_vars["GEOPM_CTL"]},
        {"GEOPM_MAX_FAN_OUT", default_vars["GEOPM_MAX_FAN_OUT"]},
        {"GEOPM_TIMEOUT", default_vars["GEOPM_TIMEOUT"]},
        {"GEOPM_DEBUG_ATTACH", m_user["GEOPM_DEBUG_ATTACH"]},
        {"GEOPM_TRACE_SIGNALS", m_user["GEOPM_TRACE_SIGNALS"]},
        {"GEOPM_REPORT_SIGNALS", m_user["GEOPM_REPORT_SIGNALS"]},
        {"GEOPM_REGION_BARRIER", m_user["GEOPM_REGION_BARRIER"]},
    };
    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, invalid_ctl)
{
    setenv("GEOPM_CTL", "program", 1);

    this->load("", "");

    EXPECT_THROW(this->pmpi_ctl(), geopm::Exception);
}

TEST_F(EnvironmentTest, c_apis)
{
    std::map<std::string, std::string> exp_vars = {
        {"GEOPM_CTL", "process"},
        {"GEOPM_DEBUG_ATTACH", "42"},
        {"GEOPM_PROFILE", "c_apis-profile"},
    };
    setenv("GEOPM_CTL", exp_vars.at("GEOPM_CTL").c_str(), 1);
    setenv("GEOPM_DEBUG_ATTACH", exp_vars.at("GEOPM_DEBUG_ATTACH").c_str(), 1);
    setenv("GEOPM_PROFILE", exp_vars.at("GEOPM_PROFILE").c_str(), 1);

    geopm_env_load();

    int test_pmpi_ctl = 0xdeadbeef;
    int test_do_profile = 0xdeadbeef;
    int test_debug_attach = 0xdeadbeef;
    (void)geopm_env_pmpi_ctl(&test_pmpi_ctl);
    (void)geopm_env_do_profile(&test_do_profile);
    (void)geopm_env_debug_attach(&test_debug_attach);
    EXPECT_EQ(GEOPM_CTL_PROCESS, test_pmpi_ctl);
    EXPECT_EQ(1, test_do_profile);
    EXPECT_EQ(exp_vars["GEOPM_DEBUG_ATTACH"], std::to_string(test_debug_attach));
}
