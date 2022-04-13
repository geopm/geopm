/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <memory>
#include <fstream>
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm_sched.h"
#include "geopm_test.hpp"
#include "Admin.hpp"
#include "geopm/Helper.hpp"

using geopm::Admin;
using testing::Return;

class AdminTest: public :: testing :: Test
{
    protected:
        void SetUp(void);
        void TearDown(void);
        std::shared_ptr<Admin> m_admin;
        int m_cpuid;
        std::string m_default_path;
        std::string m_override_path;
        std::string m_policy_path;
};

void AdminTest::SetUp(void)
{
    m_cpuid = 0x655;
    m_default_path = "environment-default.json";
    m_override_path = "environment-override.json";
    m_policy_path = "admin_test_policy.json";
    m_admin = std::make_shared<Admin>(m_default_path,
                                      m_override_path,
                                      m_cpuid);
    unlink(m_override_path.c_str());
    unlink(m_default_path.c_str());
    unlink(m_policy_path.c_str());
}

void AdminTest::TearDown(void)
{
    unlink(m_override_path.c_str());
    unlink(m_default_path.c_str());
    unlink(m_policy_path.c_str());
}

TEST_F(AdminTest, help)
{
    const char *argv[2] {"geopmadmin", "--help"};
    std::ostringstream std_out;
    std::ostringstream std_err;
    m_admin->main(2, argv, std_out, std_err);
    std::string result = std_out.str();
    ASSERT_NE(std::string::npos, result.find("Usage: geopmadmin"));
}

TEST_F(AdminTest, positional_args)
{
    const char *argv[3] {"geopmadmin", "-d", "extra-arg"};
    std::ostringstream std_out;
    std::ostringstream std_err;
    GEOPM_EXPECT_THROW_MESSAGE(m_admin->main(3, argv, std_out, std_err),
                               EINVAL, "positional argument");
}

TEST_F(AdminTest, main)
{
    const char *argv[2] {"geopmadmin", "-d"};
    std::ostringstream std_out;
    std::ostringstream std_err;
    m_admin->main(2, argv, std_out, std_err);
    std::string result = std_out.str();
    ASSERT_NE(std::string::npos, result.find("environment-default.json"));
}

TEST_F(AdminTest, two_actions)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_admin->run(true, true, false, -1),
                               EINVAL, "must be used exclusively");
    GEOPM_EXPECT_THROW_MESSAGE(m_admin->run(false, true, true, -1),
                               EINVAL, "must be used exclusively");
    GEOPM_EXPECT_THROW_MESSAGE(m_admin->run(true, false, true, -1),
                               EINVAL, "must be used exclusively");
}


TEST_F(AdminTest, config_default)
{
    std::string result = m_admin->run(true, false, false, -1);
    ASSERT_NE(std::string::npos, result.find("environment-default.json"));
}

TEST_F(AdminTest, config_override)
{
    std::string result = m_admin->run(false, true, false, -1);
    ASSERT_NE(std::string::npos, result.find("environment-override.json"));
}

TEST_F(AdminTest, allowlist)
{
    std::string result_0 = m_admin->run(false, false, true, -1);
    ASSERT_EQ(0U, result_0.find("# MSR        Write Mask           # Comment\n"));
    std::string result_1 = m_admin->run(false, false, true, m_cpuid);
    ASSERT_EQ(result_0, result_1);
}

TEST_F(AdminTest, no_options)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_admin->check_node(),
                               ENOENT, "Configuration files do not exist");
    std::ofstream override_fid(m_override_path);
    override_fid << "{\"GEOPM_REPORT\":\"geopm_report\"}";
    override_fid.close();
    std::string expect = "GEOPM CONFIGURATION\n"
                         "===================\n\n"
                         "    GEOPM_REPORT=geopm_report (override)\n";
    std::string actual = m_admin->check_node();
    ASSERT_EQ(expect, actual);
}

TEST_F(AdminTest, dup_keys)
{
    std::map<std::string, std::string> map_a;
    std::map<std::string, std::string> map_b;
    std::vector<std::string> result;

    result = Admin::dup_keys(map_a, map_b);
    ASSERT_EQ(0U, result.size());
    map_a["alpha"] = "one";
    map_b["beta"] = "two";
    result = Admin::dup_keys(map_a, map_b);
    ASSERT_EQ(0U, result.size());
    map_a["beta"] = "three";
    map_a["gamma"] = "five";
    map_b["delta"] = "four";
    result = Admin::dup_keys(map_a, map_b);
    std::vector<std::string> expect {"beta"};
    ASSERT_EQ(expect, result);
}

TEST_F(AdminTest, dup_config)
{
    std::ofstream override_fid(m_override_path);
    override_fid << "{\"GEOPM_REPORT\":\"geopm_report\"}";
    override_fid.close();
    std::ofstream default_fid(m_default_path);
    default_fid << "{\"GEOPM_REPORT\":\"geopm_report_other\"}";
    default_fid.close();
    GEOPM_EXPECT_THROW_MESSAGE(m_admin->check_node(),
                               EINVAL, "defined in both the override and default");
}

TEST_F(AdminTest, print_config)
{
    std::map<std::string, std::string> default_map = {{"GEOPM_REPORT", "default_report"}};
    std::map<std::string, std::string> override_map = {{"GEOPM_AGENT", "override_agent"},
                                                       {"GEOPM_POLICY", m_policy_path}};
    std::vector<std::string> pol_names = {"pol1", "pol2"};
    std::vector<double> pol_vals = {0.1, 0.2};
    std::string expected = "\
GEOPM CONFIGURATION\n\
===================\n\n\
    GEOPM_AGENT=override_agent (override)\n\
    GEOPM_POLICY=admin_test_policy.json (override)\n\
    GEOPM_REPORT=default_report (default)\n\
\nAGENT POLICY\n\
============\n\n\
    pol1=0.1\n\
    pol2=0.2\n";
    EXPECT_EQ(expected, m_admin->print_config(default_map, override_map, pol_names, pol_vals));
}

TEST_F(AdminTest, agent_no_policy)
{
    std::ofstream override_fid(m_override_path);
    override_fid << "{\"GEOPM_AGENT\":\"monitor\"}";
    override_fid.close();
    m_admin->check_node();
    override_fid.open(m_override_path);
    override_fid << "{\"GEOPM_AGENT\":\"monitor\","
                    " \"GEOPM_POLICY\":\"monitor_policy.json\"}";
    override_fid.close();
    std::ofstream policy_fid(m_policy_path);
    policy_fid << "{}";
    policy_fid.close();
    m_admin->check_node();
}
