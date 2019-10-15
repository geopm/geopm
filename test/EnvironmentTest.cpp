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

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <fstream>

#include "contrib/json11/json11.hpp"

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm_internal.h"
#include "Environment.hpp"
#include "Exception.hpp"
#include "Helper.hpp"

using json11::Json;
using geopm::Environment;
using geopm::EnvironmentImp;

static bool get_env(const std::string &name, std::string &env_string)
{
    bool result = false;
    char *check_string = getenv(name.c_str());
    if (check_string != NULL) {
        env_string = check_string;
        result = true;
    }
    return result;
}

class EnvironmentTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        static void vars_to_json(std::map<std::string, std::string> vars, const std::string &path);
        void expect_vars(std::map<std::string, std::string> exp_vars) const;
        void cache_env_value(const std::string &key);
        const std::string M_DEFAULT_PATH = "env_test_default.json";
        const std::string M_OVERRIDE_PATH = "env_test_override.json";
        std::map<std::string, std::string> m_user;
        std::map<std::string, int> m_pmpi_ctl_map;
        std::map<std::string, std::string> m_env_restore;
        std::unique_ptr<Environment> m_env;
};

void EnvironmentTest::vars_to_json(std::map<std::string, std::string> vars, const std::string &path)
{
    std::ofstream json_file_out(path, std::ifstream::out);
    json_file_out << Json(vars).dump();
    json_file_out.close();
}

void EnvironmentTest::cache_env_value(const std::string &env_key)
{
    std::string env_val;
    if (get_env(env_key, env_val)) {
        m_env_restore[env_key] = env_val;
    }
}

void EnvironmentTest::expect_vars(std::map<std::string, std::string> exp_vars) const
{
    EXPECT_EQ(exp_vars.find("GEOPM_TRACE") != exp_vars.end(), m_env->do_trace());
    EXPECT_EQ(exp_vars.find("GEOPM_TRACE_PROFILE") != exp_vars.end(), m_env->do_trace_profile());
    EXPECT_EQ(exp_vars.find("GEOPM_PROFILE") != exp_vars.end() ||
              exp_vars.find("GEOPM_REPORT") != exp_vars.end() ||
              exp_vars.find("GEOPM_TRACE") != exp_vars.end() ||
              exp_vars.find("GEOPM_TRACE_PROFILE") != exp_vars.end() ||
              exp_vars.find("GEOPM_CTL") != exp_vars.end(), m_env->do_profile());
    EXPECT_EQ(exp_vars["GEOPM_REPORT"], m_env->report());
    EXPECT_EQ(exp_vars["GEOPM_COMM"], m_env->comm());
    EXPECT_EQ(exp_vars["GEOPM_POLICY"], m_env->policy());
    EXPECT_EQ(exp_vars["GEOPM_AGENT"], m_env->agent());
    EXPECT_EQ(exp_vars["GEOPM_SHMKEY"], m_env->shmkey());
    EXPECT_EQ(exp_vars["GEOPM_TRACE"], m_env->trace());
    EXPECT_EQ(exp_vars["GEOPM_TRACE_PROFILE"], m_env->trace_profile());
    EXPECT_EQ(exp_vars["GEOPM_PLUGIN_PATH"], m_env->plugin_path());
    EXPECT_EQ(exp_vars["GEOPM_PROFILE"], m_env->profile());
    EXPECT_EQ(exp_vars["GEOPM_FREQUENCY_MAP"], m_env->frequency_map());
    auto it = m_pmpi_ctl_map.find(exp_vars["GEOPM_CTL"]);
    if (it != m_pmpi_ctl_map.end()) {
        EXPECT_EQ(it->second, m_env->pmpi_ctl());
    }
    EXPECT_EQ(exp_vars["GEOPM_MAX_FAN_OUT"], std::to_string(m_env->max_fan_out()));
    EXPECT_EQ(exp_vars["GEOPM_TIMEOUT"], std::to_string(m_env->timeout()));
    EXPECT_EQ(exp_vars["GEOPM_DEBUG_ATTACH"], std::to_string(m_env->debug_attach()));
    EXPECT_EQ(exp_vars["GEOPM_TRACE_SIGNALS"], m_env->trace_signals());
    EXPECT_EQ(exp_vars["GEOPM_REPORT_SIGNALS"], m_env->report_signals());
    EXPECT_EQ(exp_vars.find("GEOPM_REGION_BARRIER") != exp_vars.end(), m_env->do_region_barrier());
}

void EnvironmentTest::SetUp()
{
    // detect user vars that are present at setup
    for (const auto &key : EnvironmentImp::get_all_vars()) {
        cache_env_value(key);
    }
    m_user = {
              {"GEOPM_REPORT", "report-test_value"},
              {"GEOPM_COMM", "comm-test_value"},
              {"GEOPM_POLICY", "policy-test_value"},
              {"GEOPM_AGENT", "agent-test_value"},
              {"GEOPM_TRACE", "trace-test_value"},
              {"GEOPM_TRACE_PROFILE", "trace-profile-test_value"},
              {"GEOPM_PLUGIN_PATH", "plugin_path-test_value"},
              {"GEOPM_FREQUENCY_MAP", "hash:freq,hash:freq,hash:freq"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_DEBUG_ATTACH", "1"},
              {"GEOPM_TRACE_SIGNALS", "test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "best1,best2,best3"},
              {"GEOPM_REGION_BARRIER", std::to_string(false)},
             };

    m_pmpi_ctl_map["process"] = (int)GEOPM_CTL_PROCESS;
    m_pmpi_ctl_map["pthread"] = (int)GEOPM_CTL_PTHREAD;
}

void EnvironmentTest::TearDown()
{
    // unset all vars that could have been touched by test
    for (const auto &key : EnvironmentImp::get_all_vars()) {
        unsetenv(key.c_str());
    }
    // restore vars that were present in user env at setup
    for (const auto &kv : m_env_restore) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    (void)unlink(M_DEFAULT_PATH.c_str());
    (void)unlink(M_OVERRIDE_PATH.c_str());
}

TEST_F(EnvironmentTest, internal_defaults)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };

    m_env = geopm::make_unique<EnvironmentImp>("", "");
    std::map<std::string, std::string> exp_vars = internal_default_vars;
    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "");
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = std::string(program_invocation_name);
    exp_vars["GEOPM_SHMKEY"] = internal_default_vars["GEOPM_SHMKEY"];
    exp_vars["GEOPM_TIMEOUT"] = internal_default_vars["GEOPM_TIMEOUT"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only_do_profile)
{
    m_user["GEOPM_PROFILE"] =  "";
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "");
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = std::string(program_invocation_name);
    exp_vars["GEOPM_SHMKEY"] = internal_default_vars["GEOPM_SHMKEY"];
    exp_vars["GEOPM_TIMEOUT"] = internal_default_vars["GEOPM_TIMEOUT"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only_do_profile_name)
{
    m_user["GEOPM_PROFILE"] =  "profile-test_value";
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "");
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = "profile-test_value";
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
              {"GEOPM_TRACE_PROFILE", "default-trace-profile-test_value"},
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

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, "");
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
              {"GEOPM_TRACE_PROFILE", "override-trace-profile-test_value"},
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

    m_env = geopm::make_unique<EnvironmentImp>("", M_OVERRIDE_PATH);
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
              {"GEOPM_TRACE_PROFILE", "default-trace-profile-test_value"},
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
              {"GEOPM_TRACE_PROFILE", "override-trace-profile-test_value"},
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

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH);
    std::map<std::string, std::string> exp_vars = override_vars;
    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_default_and_override)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_SHMKEY", "/geopm-shm-" + std::to_string(geteuid())},
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

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH);
    std::map<std::string, std::string> exp_vars = {
        {"GEOPM_REPORT", m_user["GEOPM_REPORT"]},
        {"GEOPM_COMM", override_vars["GEOPM_COMM"]},
        {"GEOPM_POLICY", m_user["GEOPM_POLICY"]},
        {"GEOPM_AGENT", override_vars["GEOPM_AGENT"]},
        {"GEOPM_SHMKEY", internal_default_vars["GEOPM_SHMKEY"]},
        {"GEOPM_TRACE", m_user["GEOPM_TRACE"]},
        {"GEOPM_TRACE_PROFILE", m_user["GEOPM_TRACE_PROFILE"]},
        {"GEOPM_PLUGIN_PATH", override_vars["GEOPM_PLUGIN_PATH"]},
        {"GEOPM_PROFILE", std::string(program_invocation_name)},
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

    m_env = geopm::make_unique<EnvironmentImp>("", "");

    EXPECT_THROW(m_env->pmpi_ctl(), geopm::Exception);
}

TEST_F(EnvironmentTest, default_endpoint_user_policy)
{
    std::map<std::string, std::string> default_vars = {
        {"GEOPM_ENDPOINT", "endpoint-default_value"}
    };
    setenv("GEOPM_POLICY", "policy-user_value", 1);
    std::map<std::string, std::string> override_vars;
    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH);

    EXPECT_EQ("", m_env->endpoint());
    EXPECT_EQ("policy-user_value", m_env->policy());
}

TEST_F(EnvironmentTest, default_endpoint_user_policy_override_endpoint)
{
    std::map<std::string, std::string> default_vars = {
        {"GEOPM_ENDPOINT", "endpoint-default_value"}
    };
    setenv("GEOPM_POLICY", "policy-user_value", 1);
    std::map<std::string, std::string> override_vars = {
        {"GEOPM_ENDPOINT", "endpoint-override_value"}
    };
    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH);

    EXPECT_EQ("endpoint-override_value", m_env->endpoint());
    EXPECT_EQ("policy-user_value", m_env->policy());
}

TEST_F(EnvironmentTest, user_policy_and_endpoint)
{
    std::map<std::string, std::string> default_vars;
    setenv("GEOPM_POLICY", "policy-user_value", 1);
    setenv("GEOPM_ENDPOINT", "endpoint-user_value", 1);
    std::map<std::string, std::string> override_vars;

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH);

    EXPECT_EQ("endpoint-user_value", m_env->endpoint());
    EXPECT_EQ("policy-user_value", m_env->policy());
}
