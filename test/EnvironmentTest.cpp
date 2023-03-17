/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <fstream>
#include <set>

#include "geopm/json11.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_error.h"
#include "geopm_field.h"
#include "geopm_topo.h"  // for geopm_domain_e
#include "geopm_test.hpp" // for GEOPM_EXPECT_THROW_MESSAGE
#include "Environment.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

#include "MockPlatformIO.hpp"

using json11::Json;
using geopm::Environment;
using geopm::EnvironmentImp;
using testing::_;
using testing::Return;

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
        void check_trace_report_signals(const std::vector<std::pair<std::string, int> >& expected_trace_signals,
                                        const std::vector<std::pair<std::string, int> >& expected_report_signals) const;
        void cache_env_value(const std::string &key);
        const std::string M_DEFAULT_PATH = "env_test_default.json";
        const std::string M_OVERRIDE_PATH = "env_test_override.json";
        std::map<std::string, std::string> m_user;
        std::map<std::string, int> m_pmpi_ctl_map;
        std::map<std::string, std::string> m_env_restore;
        std::vector<std::pair<std::string, int> > m_trace_signals;
        std::vector<std::pair<std::string, int> > m_report_signals;
        std::unique_ptr<Environment> m_env;
        MockPlatformIO m_platform_io;
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
    EXPECT_EQ(exp_vars["GEOPM_TRACE"], m_env->trace());
    EXPECT_EQ(exp_vars["GEOPM_TRACE_PROFILE"], m_env->trace_profile());
    EXPECT_EQ("\"" + exp_vars["GEOPM_PROFILE"] + "\"", m_env->profile());
    EXPECT_EQ(exp_vars["GEOPM_FREQUENCY_MAP"], m_env->frequency_map());
    auto it = m_pmpi_ctl_map.find(exp_vars["GEOPM_CTL"]);
    if (it != m_pmpi_ctl_map.end()) {
        EXPECT_EQ(it->second, m_env->pmpi_ctl());
    }
    EXPECT_EQ(exp_vars["GEOPM_MAX_FAN_OUT"], std::to_string(m_env->max_fan_out()));
    EXPECT_EQ(exp_vars["GEOPM_TIMEOUT"], std::to_string(m_env->timeout()));
    EXPECT_EQ(exp_vars["GEOPM_DEBUG_ATTACH"], std::to_string(m_env->debug_attach_process()));
}

void EnvironmentTest::check_trace_report_signals(const std::vector<std::pair<std::string, int> >& expected_trace_signals,
                                                 const std::vector<std::pair<std::string, int> >& expected_report_signals) const
{
    auto actual_trace_signals  = m_env->trace_signals();
    auto actual_report_signals = m_env->report_signals();

    EXPECT_EQ(actual_trace_signals.size(), expected_trace_signals.size());
    EXPECT_EQ(actual_report_signals.size(), expected_report_signals.size());
    size_t trace_signals_size = actual_trace_signals.size();
    size_t report_signals_size = actual_report_signals.size();
    size_t counter = 0;

    for (counter = 0; counter < trace_signals_size; ++counter) {
        EXPECT_EQ(actual_trace_signals[counter].first, expected_trace_signals[counter].first);
        EXPECT_EQ(actual_trace_signals[counter].second, expected_trace_signals[counter].second);
    }

    for (counter = 0; counter < report_signals_size; ++counter) {
        EXPECT_EQ(actual_report_signals[counter].first, expected_report_signals[counter].first);
        EXPECT_EQ(actual_report_signals[counter].second, expected_report_signals[counter].second);
    }
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
              {"GEOPM_FREQUENCY_MAP", "hash:freq,hash:freq,hash:freq"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_DEBUG_ATTACH", "1"},
              {"GEOPM_TRACE_SIGNALS", "test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "best1,best2,best3"},
             };
    // Contains the same information as the GEOPM_TRACE_SIGNALS and GEOPM_REPORT_SIGNALS,
    // but in the format of a data structure, as inputs to check_trace_report_signals()
    m_trace_signals = {
        {"test1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    m_report_signals = {
        {"best1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };

    m_pmpi_ctl_map["process"] = (int)Environment::M_CTL_PROCESS;
    m_pmpi_ctl_map["pthread"] = (int)Environment::M_CTL_PTHREAD;

    std::set<std::string> valid_signal_names = {
        "CPUINFO::FREQ_MAX",
        "CPUINFO::FREQ_MIN",
        "CPUINFO::FREQ_STEP",
        "CPUINFO::FREQ_STICKER",
        "CPU_FREQUENCY_MIN_AVAIL",
        "CPU_FREQUENCY_STEP",
        "CPU_FREQUENCY_STICKER",
        "TIME",
        "TIME::ELAPSED",
        "test1",
        "default-test1",
        "override-test1",
        "best1",
        "default-best1",
        "override-best1",
        "test2",
        "best2",
        "test3",
        "best3",
    };

    EXPECT_CALL(m_platform_io, signal_names())
        .WillRepeatedly(Return(valid_signal_names));
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
#ifdef GEOPM_ENABLE_MPI
              {"GEOPM_COMM", "MPIComm"},
#else
              {"GEOPM_COMM", "NullComm"},
#endif
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);
    std::map<std::string, std::string> exp_vars = internal_default_vars;
    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = std::string(program_invocation_name);
    exp_vars["GEOPM_TIMEOUT"] = internal_default_vars["GEOPM_TIMEOUT"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only_do_profile)
{
    m_user["GEOPM_PROFILE"] =  "";
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = std::string(program_invocation_name);
    exp_vars["GEOPM_TIMEOUT"] = internal_default_vars["GEOPM_TIMEOUT"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only_do_profile_custom)
{
    m_user["GEOPM_PROFILE"] =  "\nThat's \"all\" folks ";
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = "That's all folks ";
    exp_vars["GEOPM_TIMEOUT"] = internal_default_vars["GEOPM_TIMEOUT"];

    expect_vars(exp_vars);
}

TEST_F(EnvironmentTest, user_only_do_profile_name)
{
    m_user["GEOPM_PROFILE"] =  "profile-test_value";
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "30"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
             };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);
    std::map<std::string, std::string> exp_vars = m_user;
    exp_vars["GEOPM_PROFILE"] = "profile-test_value";
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
              {"GEOPM_TRACE", "default-trace-test_value"},
              {"GEOPM_TRACE_PROFILE", "default-trace-profile-test_value"},
              {"GEOPM_PROFILE", "default-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "default-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "pthread"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "0"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "default-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "default-best1,best2,best3"},
             };
    m_trace_signals = {
        {"default-test1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    m_report_signals = {
        {"default-best1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    vars_to_json(default_vars, M_DEFAULT_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, "", &m_platform_io);
    std::map<std::string, std::string> exp_vars = default_vars;

    expect_vars(exp_vars);
    check_trace_report_signals(m_trace_signals, m_report_signals);
}

TEST_F(EnvironmentTest, override_only)
{
    std::map<std::string, std::string> override_vars = {
              {"GEOPM_REPORT", "override-report-test_value"},
              {"GEOPM_COMM", "override-comm-test_value"},
              {"GEOPM_POLICY", "override-policy-test_value"},
              {"GEOPM_AGENT", "override-agent-test_value"},
              {"GEOPM_TRACE", "override-trace-test_value"},
              {"GEOPM_TRACE_PROFILE", "override-trace-profile-test_value"},
              {"GEOPM_PROFILE", "override-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "override-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "process"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "15"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "override-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "override-best1,best2,best3"},
             };
    m_trace_signals = {
        {"override-test1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    m_report_signals = {
        {"override-best1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>("", M_OVERRIDE_PATH, &m_platform_io);
    std::map<std::string, std::string> exp_vars = override_vars;

    expect_vars(exp_vars);
    check_trace_report_signals(m_trace_signals, m_report_signals);
}

TEST_F(EnvironmentTest, default_and_override)
{
    std::map<std::string, std::string> default_vars = {
              {"GEOPM_REPORT", "default-report-test_value"},
              {"GEOPM_COMM", "default-comm-test_value"},
              {"GEOPM_POLICY", "default-policy-test_value"},
              {"GEOPM_AGENT", "default-agent-test_value"},
              {"GEOPM_TRACE", "default-trace-test_value"},
              {"GEOPM_TRACE_PROFILE", "default-trace-profile-test_value"},
              {"GEOPM_PROFILE", "default-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "default-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "pthread"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "0"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "default-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "default-best1,best2,best3"},
             };
    std::map<std::string, std::string> override_vars = {
              {"GEOPM_REPORT", "override-report-test_value"},
              {"GEOPM_COMM", "override-comm-test_value"},
              {"GEOPM_POLICY", "override-policy-test_value"},
              {"GEOPM_AGENT", "override-agent-test_value"},
              {"GEOPM_TRACE", "override-trace-test_value"},
              {"GEOPM_TRACE_PROFILE", "override-trace-profile-test_value"},
              {"GEOPM_PROFILE", "override-profile-test_value"},
              {"GEOPM_FREQUENCY_MAP", "override-hash:freq,hash:freq,hash:freq"},
              {"GEOPM_CTL", "process"},
              {"GEOPM_MAX_FAN_OUT", "16"},
              {"GEOPM_TIMEOUT", "15"},
              {"GEOPM_DEBUG_ATTACH", "-1"},
              {"GEOPM_TRACE_SIGNALS", "override-test1,test2,test3"},
              {"GEOPM_REPORT_SIGNALS", "override-best1,best2,best3"},
             };

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    std::map<std::string, std::string> exp_vars = override_vars;
    m_trace_signals = {
        {"override-test1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"test3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    m_report_signals = {
        {"override-best1", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best2", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"best3", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };

    // Uses the override_vars
    expect_vars(exp_vars);
    // Uses the m_trace_signals, m_report_signals corresponding to the override_vars
    check_trace_report_signals(m_trace_signals, m_report_signals);
}

TEST_F(EnvironmentTest, user_default_and_override)
{
    std::map<std::string, std::string> internal_default_vars = {
              {"GEOPM_COMM", "MPIComm"},
              {"GEOPM_AGENT", "monitor"},
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
              {"GEOPM_CTL", "process"},
    };
    for (const auto &kv : m_user) {
        setenv(kv.first.c_str(), kv.second.c_str(), 1);
    }
    // default
    vars_to_json(default_vars, M_DEFAULT_PATH);
    // override
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);
    std::map<std::string, std::string> exp_vars = {
        {"GEOPM_REPORT", m_user["GEOPM_REPORT"]},
        {"GEOPM_COMM", override_vars["GEOPM_COMM"]},
        {"GEOPM_POLICY", m_user["GEOPM_POLICY"]},
        {"GEOPM_AGENT", override_vars["GEOPM_AGENT"]},
        {"GEOPM_TRACE", m_user["GEOPM_TRACE"]},
        {"GEOPM_TRACE_PROFILE", m_user["GEOPM_TRACE_PROFILE"]},
        {"GEOPM_PROFILE", std::string(program_invocation_name)},
        {"GEOPM_FREQUENCY_MAP", m_user["GEOPM_FREQUENCY_MAP"]},
        {"GEOPM_CTL", override_vars["GEOPM_CTL"]},
        {"GEOPM_MAX_FAN_OUT", default_vars["GEOPM_MAX_FAN_OUT"]},
        {"GEOPM_TIMEOUT", default_vars["GEOPM_TIMEOUT"]},
        {"GEOPM_DEBUG_ATTACH", m_user["GEOPM_DEBUG_ATTACH"]},
        {"GEOPM_TRACE_SIGNALS", m_user["GEOPM_TRACE_SIGNALS"]},
        {"GEOPM_REPORT_SIGNALS", m_user["GEOPM_REPORT_SIGNALS"]},
    };
    // Uses the m_user values from the SetUp() method.
    expect_vars(exp_vars);
    // Uses the m_trace_signals, m_report_signals values from the SetUp() method.
    check_trace_report_signals(m_trace_signals, m_report_signals);
}

TEST_F(EnvironmentTest, invalid_ctl)
{
    setenv("GEOPM_CTL", "program", 1);

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);

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

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

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

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

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

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    EXPECT_EQ("endpoint-user_value", m_env->endpoint());
    EXPECT_EQ("policy-user_value", m_env->policy());
}

TEST_F(EnvironmentTest, user_disable_ompt)
{
    std::map<std::string, std::string> default_vars;
    setenv("GEOPM_OMPT_DISABLE", "is_set", 1);
    std::map<std::string, std::string> override_vars;

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    EXPECT_FALSE(m_env->do_ompt());
}

TEST_F(EnvironmentTest, record_filter_on)
{
    std::map<std::string, std::string> default_vars;
    setenv("GEOPM_RECORD_FILTER", "proxy_epoch,0xabcd1234", 1);
    std::map<std::string, std::string> override_vars;

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    EXPECT_TRUE(m_env->do_record_filter());
    EXPECT_EQ("proxy_epoch,0xabcd1234", m_env->record_filter());
}

TEST_F(EnvironmentTest, record_filter_off)
{
    std::map<std::string, std::string> default_vars;
    std::map<std::string, std::string> override_vars;

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    EXPECT_FALSE(m_env->do_record_filter());
    EXPECT_EQ("", m_env->record_filter());
}

TEST_F(EnvironmentTest, init_control_set)
{
    std::map<std::string, std::string> default_vars;
    setenv("GEOPM_INIT_CONTROL", "/tmp/test_input", 1);
    std::map<std::string, std::string> override_vars;

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    EXPECT_TRUE(m_env->do_init_control());
    EXPECT_EQ("/tmp/test_input", m_env->init_control());
}

TEST_F(EnvironmentTest, init_control_unset)
{
    std::map<std::string, std::string> default_vars;
    std::map<std::string, std::string> override_vars;

    vars_to_json(default_vars, M_DEFAULT_PATH);
    vars_to_json(override_vars, M_OVERRIDE_PATH);

    m_env = geopm::make_unique<EnvironmentImp>(M_DEFAULT_PATH, M_OVERRIDE_PATH, &m_platform_io);

    EXPECT_FALSE(m_env->do_init_control());
    EXPECT_EQ("", m_env->init_control());
}

TEST_F(EnvironmentTest, signal_parser)
{
    std::vector<std::pair<std::string, int> >& expected_signals = m_trace_signals;
    std::vector<std::pair<std::string, int> > actual_signals;
    std::string environment_variable_contents;

    m_env = geopm::make_unique<EnvironmentImp>("", "", &m_platform_io);

    expected_signals = {
        {"CPU_FREQUENCY_MIN_AVAIL", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"CPUINFO::FREQ_STEP", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"TIME", geopm_domain_e::GEOPM_DOMAIN_BOARD}
    };
    environment_variable_contents = "CPU_FREQUENCY_MIN_AVAIL,CPUINFO::FREQ_STEP,TIME";
    actual_signals = dynamic_cast<EnvironmentImp*>(m_env.get())->signal_parser(environment_variable_contents);
    EXPECT_EQ(expected_signals, actual_signals);

    expected_signals = {
        {"CPU_FREQUENCY_MIN_AVAIL", geopm_domain_e::GEOPM_DOMAIN_BOARD},
        {"CPUINFO::FREQ_STEP", geopm_domain_e::GEOPM_DOMAIN_PACKAGE},
        {"TIME", geopm_domain_e::GEOPM_DOMAIN_CORE}
    };
    environment_variable_contents = "CPU_FREQUENCY_MIN_AVAIL,CPUINFO::FREQ_STEP@package,TIME@core";
    actual_signals = dynamic_cast<EnvironmentImp*>(m_env.get())->signal_parser(environment_variable_contents);
    EXPECT_EQ(expected_signals, actual_signals);

    expected_signals = {
        {"CPUINFO::FREQ_STEP", geopm_domain_e::GEOPM_DOMAIN_INVALID},
        {"TIME", geopm_domain_e::GEOPM_DOMAIN_INVALID}
    };
    environment_variable_contents = "CPUINFO::FREQ_STEP@invalid,TIME@invalid";
    GEOPM_EXPECT_THROW_MESSAGE(
        dynamic_cast<EnvironmentImp*>(m_env.get())->signal_parser(environment_variable_contents),
        GEOPM_ERROR_INVALID,
        "PlatformTopo::domain_name_to_type(): unrecognized domain_name: invalid"
    );

    environment_variable_contents = "CPU_FREQUENCY_MIN_AVAIL,CPUINFO::FREQ_STEP@package@core,TIME@core";
    GEOPM_EXPECT_THROW_MESSAGE(
        dynamic_cast<EnvironmentImp*>(m_env.get())->signal_parser(environment_variable_contents),
        GEOPM_ERROR_INVALID,
        "EnvironmentImp::signal_parser(): Environment trace extension contains signals with multiple \"@\" characters."
    );

    environment_variable_contents = "CPU_FREQUENCY_MIN_AVAIL,NUM_VACUUM_TUBES@package,TIME@core";
    GEOPM_EXPECT_THROW_MESSAGE(
        dynamic_cast<EnvironmentImp*>(m_env.get())->signal_parser(environment_variable_contents),
        GEOPM_ERROR_INVALID,
        "Invalid signal : NUM_VACUUM_TUBES"
    );
}
