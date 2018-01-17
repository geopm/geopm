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
#include <unistd.h>
#include <fstream>

#include "gtest/gtest.h"
#include "geopm_policy.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "GlobalPolicy.hpp"

class GlobalPolicyTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        std::string m_path;
};

class GlobalPolicyTestShmem: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        std::string m_path;
};

void GlobalPolicyTest::SetUp()
{
    m_path.assign("./policy.conf");
}

void GlobalPolicyTest::TearDown()
{
    remove(m_path.c_str());
}

void GlobalPolicyTestShmem::SetUp()
{
    m_path.assign("/GlobalPolicyTestShmem-");
    m_path.append(std::to_string(getpid()));
}

void GlobalPolicyTestShmem::TearDown()
{
    unlink(m_path.c_str());
}

TEST_F(GlobalPolicyTest, mode_tdp_balance_static)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->mode(GEOPM_POLICY_MODE_TDP_BALANCE_STATIC);
    policy->tdp_percent(75.0);
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->tdp_percent(34.0);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_DOUBLE_EQ(34.0, policy->tdp_percent());

    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_TDP_BALANCE_STATIC, policy->mode());
    EXPECT_DOUBLE_EQ(75.0, policy->tdp_percent());
    delete policy;
}

TEST_F(GlobalPolicyTest, mode_freq_uniform_static)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->frequency_mhz(1800);
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC);
    policy->frequency_mhz(3400);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC, policy->mode());
    EXPECT_EQ(3400, policy->frequency_mhz());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_DOUBLE_EQ(1800, policy->frequency_mhz());
    delete policy;
}

TEST_F(GlobalPolicyTest, mode_freq_hybrid_static)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->mode(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC);
    policy->frequency_mhz(1800);
    policy->num_max_perf(16);
    policy->affinity(GEOPM_POLICY_AFFINITY_SCATTER);
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->frequency_mhz(3600);
    policy->num_max_perf(42);
    policy->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_EQ(3600, policy->frequency_mhz());
    EXPECT_EQ(42, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_COMPACT, policy->affinity());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC, policy->mode());
    EXPECT_EQ(1800, policy->frequency_mhz());
    EXPECT_EQ(16, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_SCATTER, policy->affinity());
    delete policy;
}

TEST_F(GlobalPolicyTest, mode_perf_balance_dynamic)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC);
    policy->budget_watts(75500);
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC);
    policy->budget_watts(850);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC, policy->mode());
    EXPECT_EQ(850, policy->budget_watts());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC, policy->mode());
    EXPECT_DOUBLE_EQ(75500, policy->budget_watts());
    delete policy;
}

TEST_F(GlobalPolicyTest, mode_freq_uniform_dynamic)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC);
    policy->budget_watts(1025);
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->budget_watts(625);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_EQ(625, policy->budget_watts());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC, policy->mode());
    EXPECT_DOUBLE_EQ(1025, policy->budget_watts());
    delete policy;
}

TEST_F(GlobalPolicyTest, mode_freq_hybrid_dynamic)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC);
    policy->budget_watts(9612);
    policy->num_max_perf(24);
    policy->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->budget_watts(4242);
    policy->num_max_perf(86);
    policy->affinity(GEOPM_POLICY_AFFINITY_SCATTER);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_EQ(4242, policy->budget_watts());
    EXPECT_EQ(86, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_SCATTER, policy->affinity());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC, policy->mode());
    EXPECT_EQ(9612, policy->budget_watts());
    EXPECT_EQ(24, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_COMPACT, policy->affinity());
    delete policy;
}

TEST_F(GlobalPolicyTest, plugin_strings)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    // write values to file
    policy->mode(GEOPM_POLICY_MODE_DYNAMIC);
    policy->budget_watts(9612);
    policy->num_max_perf(24);
    policy->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    policy->tree_decider("test_tree_decider");
    policy->leaf_decider("test_leaf_decider");
    policy->platform("test_platform");
    policy->write();
    delete policy;

    policy = new geopm::GlobalPolicy(m_path, "");
    //overwrite local values
    policy->tree_decider("new_tree_decider");
    policy->leaf_decider("new_leaf_decider");
    policy->platform("new_platform");
    ASSERT_STREQ("new_tree_decider", policy->tree_decider().c_str());
    ASSERT_STREQ("new_leaf_decider", policy->leaf_decider().c_str());
    ASSERT_STREQ("new_platform", policy->platform().c_str());
    //read saved values back
    policy->read();
    ASSERT_STREQ("test_tree_decider", policy->tree_decider().c_str());
    ASSERT_STREQ("test_leaf_decider", policy->leaf_decider().c_str());
    ASSERT_STREQ("test_platform", policy->platform().c_str());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, mode_tdp_balance_static)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_TDP_BALANCE_STATIC);
    policy->tdp_percent(75.0);
    policy->write();
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->tdp_percent(34.0);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_DOUBLE_EQ(34.0, policy->tdp_percent());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_TDP_BALANCE_STATIC, policy->mode());
    EXPECT_DOUBLE_EQ(75.0, policy->tdp_percent());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, mode_freq_uniform_static)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->frequency_mhz(1800);
    policy->write();
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC);
    policy->frequency_mhz(3400);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC, policy->mode());
    EXPECT_EQ(3400, policy->frequency_mhz());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_DOUBLE_EQ(1800, policy->frequency_mhz());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, mode_freq_hybrid_static)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC);
    policy->frequency_mhz(1800);
    policy->num_max_perf(16);
    policy->affinity(GEOPM_POLICY_AFFINITY_SCATTER);
    policy->write();
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->frequency_mhz(3600);
    policy->num_max_perf(42);
    policy->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_EQ(3600, policy->frequency_mhz());
    EXPECT_EQ(42, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_COMPACT, policy->affinity());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_HYBRID_STATIC, policy->mode());
    EXPECT_EQ(1800, policy->frequency_mhz());
    EXPECT_EQ(16, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_SCATTER, policy->affinity());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, mode_perf_balance_dynamic)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC);
    policy->budget_watts(75500);
    policy->write();
    //overwrite local values
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC);
    policy->budget_watts(850);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC, policy->mode());
    EXPECT_EQ(850, policy->budget_watts());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC, policy->mode());
    EXPECT_DOUBLE_EQ(75500, policy->budget_watts());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, mode_freq_uniform_dynamic)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC);
    policy->budget_watts(1025);
    policy->write();
    //overwrite local values
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->budget_watts(625);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_EQ(625, policy->budget_watts());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC, policy->mode());
    EXPECT_DOUBLE_EQ(1025, policy->budget_watts());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, mode_freq_hybrid_dynamic)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC);
    policy->budget_watts(9612);
    policy->num_max_perf(24);
    policy->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    policy->write();
    //overwrite local values
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->budget_watts(4242);
    policy->num_max_perf(86);
    policy->affinity(GEOPM_POLICY_AFFINITY_SCATTER);
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC, policy->mode());
    EXPECT_EQ(4242, policy->budget_watts());
    EXPECT_EQ(86, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_SCATTER, policy->affinity());
    //read saved values back
    policy->read();
    EXPECT_EQ(GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC, policy->mode());
    EXPECT_EQ(9612, policy->budget_watts());
    EXPECT_EQ(24, policy->num_max_perf());
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_COMPACT, policy->affinity());
    delete policy;
}

TEST_F(GlobalPolicyTestShmem, plugin_strings)
{
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy(m_path, m_path);
    // write values to file
    policy->mode(GEOPM_POLICY_MODE_DYNAMIC);
    policy->budget_watts(9612);
    policy->num_max_perf(24);
    policy->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    policy->tree_decider("test_tree_decider");
    policy->leaf_decider("test_leaf_decider");
    policy->platform("test_platform");
    policy->write();
    //overwrite local values
    policy->tree_decider("new_tree_decider");
    policy->leaf_decider("new_leaf_decider");
    policy->platform("new_platform");
    ASSERT_STREQ("new_tree_decider", policy->tree_decider().c_str());
    ASSERT_STREQ("new_leaf_decider", policy->leaf_decider().c_str());
    ASSERT_STREQ("new_platform", policy->platform().c_str());
    //read saved values back
    policy->read();
    ASSERT_STREQ("test_tree_decider", policy->tree_decider().c_str());
    ASSERT_STREQ("test_leaf_decider", policy->leaf_decider().c_str());
    ASSERT_STREQ("test_platform", policy->platform().c_str());
    delete policy;
}

TEST_F(GlobalPolicyTest, invalid_policy)
{
    // mismatched mode and decider
    geopm::GlobalPolicy *policy = new geopm::GlobalPolicy("", m_path);
    policy->tree_decider("power_balancing");
    policy->leaf_decider("power_governing");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_STATIC);
    policy->frequency_mhz(1800);
    EXPECT_THROW(policy->write(), geopm::Exception);
    policy->tree_decider("static_policy");
    policy->leaf_decider("static_policy");
    policy->mode(GEOPM_POLICY_MODE_FREQ_UNIFORM_DYNAMIC);
    policy->budget_watts(850);
    EXPECT_THROW(policy->write(), geopm::Exception);
    delete policy;

    // read invalid file
    std::string input_config = "test/invalid_policy.json";
    std::ofstream config_stream(input_config);
    config_stream << "{ \"mode\": \"perf_balance_dynamic\", "
                  << "  \"options\": { \"tree_decider\": \"static_policy\", "
                  << "                 \"leaf_decider\": \"static_policy\", "
                  << "                 \"platform\": \"rapl\", "
                  << "                 \"power_budget\": 800 } }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // missing or empty policy file
    EXPECT_THROW(geopm::GlobalPolicy missing_file(input_config, ""), geopm::Exception);
    config_stream.open(input_config);
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy empty_file(input_config, ""), geopm::Exception);

    // malformed json file
    config_stream.open(input_config);
    config_stream << "{bad json]";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // unknown root key
    config_stream.open(input_config);
    config_stream << "{\"unknown\":1}";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // options must be object
    config_stream.open(input_config);
    config_stream << "{\"options\":1}";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // tdp_percent must be double
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"tdp_percent\": \"percent\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // cpu_mhz must be integer
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"cpu_mhz\": \"percent\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"cpu_mhz\": \"5.5\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // num_cpu_max_perf must be integer
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"num_cpu_max_perf\": \"number\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"num_cpu_max_perf\": \"5.5\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // affinity must be string
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"affinity\": 12} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // affinity string must be "compact" or "scatter"
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"affinity\": \"unknown\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // power_budget must be integer
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"power_budget\": \"number\"} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"power_budget\": 77.77} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // tree_decider must be string
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"tree_decider\": 12} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // leaf_decider must be string
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"leaf_decider\": 12} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // platform must be string
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"platform\": 12} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // unknown option key
    config_stream.open(input_config);
    config_stream << "{\"options\": {\"unknown\": 2} }";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // mode must be string
    config_stream.open(input_config);
    config_stream << "{\"mode\": 5}";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    // invalid mode string
    config_stream.open(input_config);
    config_stream << "{\"mode\": \"unknown\"}";
    config_stream.close();
    EXPECT_THROW(geopm::GlobalPolicy malformed_file(input_config, ""), geopm::Exception);

    std::remove(input_config.c_str());
}

TEST_F(GlobalPolicyTest, c_interface)
{
    struct geopm_policy_c *policy;
    const char path[] = {"/tmp/policy.conf"};

    EXPECT_EQ(0, geopm_policy_create("", path, &policy));
    EXPECT_EQ(0, geopm_policy_power(policy, 2500));
    EXPECT_EQ(0, geopm_policy_mode(policy, GEOPM_POLICY_MODE_DYNAMIC));
    EXPECT_EQ(0, geopm_policy_cpu_freq(policy, 2200));
    EXPECT_EQ(0, geopm_policy_full_perf(policy, 8));
    EXPECT_EQ(0, geopm_policy_tdp_percent(policy, 60));
    EXPECT_EQ(0, geopm_policy_affinity(policy, GEOPM_POLICY_AFFINITY_SCATTER));
    EXPECT_EQ(0, geopm_policy_goal(policy, GEOPM_POLICY_GOAL_CPU_EFFICIENCY));
    EXPECT_EQ(0, geopm_policy_tree_decider(policy, "test_tree_decider"));
    EXPECT_EQ(0, geopm_policy_tree_decider(policy, "test_leaf_decider"));
    EXPECT_EQ(0, geopm_policy_tree_decider(policy, "test_platform"));
    EXPECT_EQ(0, geopm_policy_write(policy));
    EXPECT_EQ(0, geopm_policy_destroy(policy));
    EXPECT_EQ(0, remove(path));
}

TEST_F(GlobalPolicyTest, negative_c_interface)
{
    struct geopm_policy_c *policy = NULL;

    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_power(policy, 2500));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_mode(policy, GEOPM_POLICY_MODE_FREQ_HYBRID_DYNAMIC));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_cpu_freq(policy, 2200));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_full_perf(policy, 8));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_tdp_percent(policy, 60));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_affinity(policy, GEOPM_POLICY_AFFINITY_SCATTER));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_goal(policy, GEOPM_POLICY_GOAL_CPU_EFFICIENCY));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_tree_decider(policy, "test_tree_decider"));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_tree_decider(policy, "test_leaf_decider"));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_tree_decider(policy, "test_platform"));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_write(policy));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_destroy(policy));
}
