/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <iostream>

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
        geopm::GlobalPolicy *m_policy;
        std::string m_path;
};

class GlobalPolicyTestShmem: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        geopm::GlobalPolicy *m_policy;
};

void GlobalPolicyTest::SetUp()
{
    m_path = "./policy.conf";
    m_policy = new geopm::GlobalPolicy(m_path, m_path);
}

void GlobalPolicyTest::TearDown()
{
    delete m_policy;
    unlink(m_path.c_str());
}

void GlobalPolicyTestShmem::SetUp()
{
    std::string key("/GlobalPolicyTestShmem-");
    key.append(std::to_string(getpid()));
    m_policy = new geopm::GlobalPolicy(key, key);
}

void GlobalPolicyTestShmem::TearDown()
{
    delete m_policy;
}

TEST_F(GlobalPolicyTest, mode_tdp_balance_static)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_TDP_BALANCE_STATIC);
    m_policy->tdp_percent(75);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->tdp_percent(34);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(34, m_policy->tdp_percent());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_TDP_BALANCE_STATIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(75, m_policy->tdp_percent());
}

TEST_F(GlobalPolicyTest, mode_freq_uniform_static)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->frequency_mhz(1800);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_HYBRID_STATIC);
    m_policy->frequency_mhz(3400);
    EXPECT_EQ(GEOPM_MODE_FREQ_HYBRID_STATIC, m_policy->mode());
    EXPECT_EQ(3400, m_policy->frequency_mhz());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(1800, m_policy->frequency_mhz());
}

TEST_F(GlobalPolicyTest, mode_freq_hybrid_static)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_HYBRID_STATIC);
    m_policy->frequency_mhz(1800);
    m_policy->num_max_perf(16);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->frequency_mhz(3600);
    m_policy->num_max_perf(42);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(3600, m_policy->frequency_mhz());
    EXPECT_EQ(42, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT, m_policy->affinity());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_HYBRID_STATIC, m_policy->mode());
    EXPECT_EQ(1800, m_policy->frequency_mhz());
    EXPECT_EQ(16, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER, m_policy->affinity());
}

TEST_F(GlobalPolicyTest, mode_perf_balance_dynamic)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_PERF_BALANCE_DYNAMIC);
    m_policy->budget_watts(75500);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->budget_watts(850);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(850, m_policy->budget_watts());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_PERF_BALANCE_DYNAMIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(75500, m_policy->budget_watts());
}

TEST_F(GlobalPolicyTest, mode_freq_uniform_dynamic)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_DYNAMIC);
    m_policy->budget_watts(1025);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->budget_watts(625);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(625, m_policy->budget_watts());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_DYNAMIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(1025, m_policy->budget_watts());
}

TEST_F(GlobalPolicyTest, mode_freq_hybrid_dynamic)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_HYBRID_DYNAMIC);
    m_policy->budget_watts(9612);
    m_policy->num_max_perf(24);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->budget_watts(4242);
    m_policy->num_max_perf(86);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(4242, m_policy->budget_watts());
    EXPECT_EQ(86, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER, m_policy->affinity());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_HYBRID_DYNAMIC, m_policy->mode());
    EXPECT_EQ(9612, m_policy->budget_watts());
    EXPECT_EQ(24, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT, m_policy->affinity());
}

TEST_F(GlobalPolicyTestShmem, mode_tdp_balance_static)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_TDP_BALANCE_STATIC);
    m_policy->tdp_percent(75);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->tdp_percent(34);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(34, m_policy->tdp_percent());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_TDP_BALANCE_STATIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(75, m_policy->tdp_percent());
}

TEST_F(GlobalPolicyTestShmem, mode_freq_uniform_static)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->frequency_mhz(1800);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_HYBRID_STATIC);
    m_policy->frequency_mhz(3400);
    EXPECT_EQ(GEOPM_MODE_FREQ_HYBRID_STATIC, m_policy->mode());
    EXPECT_EQ(3400, m_policy->frequency_mhz());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(1800, m_policy->frequency_mhz());
}

TEST_F(GlobalPolicyTestShmem, mode_freq_hybrid_static)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_HYBRID_STATIC);
    m_policy->frequency_mhz(1800);
    m_policy->num_max_perf(16);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->frequency_mhz(3600);
    m_policy->num_max_perf(42);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(3600, m_policy->frequency_mhz());
    EXPECT_EQ(42, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT, m_policy->affinity());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_HYBRID_STATIC, m_policy->mode());
    EXPECT_EQ(1800, m_policy->frequency_mhz());
    EXPECT_EQ(16, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER, m_policy->affinity());
}

TEST_F(GlobalPolicyTestShmem, mode_perf_balance_dynamic)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_PERF_BALANCE_DYNAMIC);
    m_policy->budget_watts(75500);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->budget_watts(850);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(850, m_policy->budget_watts());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_PERF_BALANCE_DYNAMIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(75500, m_policy->budget_watts());
}

TEST_F(GlobalPolicyTestShmem, mode_freq_uniform_dynamic)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_DYNAMIC);
    m_policy->budget_watts(1025);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->budget_watts(625);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(625, m_policy->budget_watts());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_DYNAMIC, m_policy->mode());
    EXPECT_DOUBLE_EQ(1025, m_policy->budget_watts());
}

TEST_F(GlobalPolicyTestShmem, mode_freq_hybrid_dynamic)
{
    // write values to file
    m_policy->mode(GEOPM_MODE_FREQ_HYBRID_DYNAMIC);
    m_policy->budget_watts(9612);
    m_policy->num_max_perf(24);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT);
    m_policy->write();
    //overwrite local values
    m_policy->mode(GEOPM_MODE_FREQ_UNIFORM_STATIC);
    m_policy->budget_watts(4242);
    m_policy->num_max_perf(86);
    m_policy->affinity(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER);
    EXPECT_EQ(GEOPM_MODE_FREQ_UNIFORM_STATIC, m_policy->mode());
    EXPECT_EQ(4242, m_policy->budget_watts());
    EXPECT_EQ(86, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER, m_policy->affinity());
    //read saved values back
    m_policy->read();
    EXPECT_EQ(GEOPM_MODE_FREQ_HYBRID_DYNAMIC, m_policy->mode());
    EXPECT_EQ(9612, m_policy->budget_watts());
    EXPECT_EQ(24, m_policy->num_max_perf());
    EXPECT_EQ(GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_COMPACT, m_policy->affinity());
}

TEST_F(GlobalPolicyTest, c_interface)
{
    struct geopm_policy_c *policy;
    const char path[] = {"/tmp/policy.conf"};

    EXPECT_EQ(0, geopm_policy_create("", path, &policy));
    EXPECT_EQ(0, geopm_policy_power(policy, 2500));
    EXPECT_EQ(0, geopm_policy_mode(policy, GEOPM_MODE_FREQ_HYBRID_DYNAMIC));
    EXPECT_EQ(0, geopm_policy_cpu_freq(policy, 2200));
    EXPECT_EQ(0, geopm_policy_full_perf(policy, 8));
    EXPECT_EQ(0, geopm_policy_tdp_percent(policy, 60));
    EXPECT_EQ(0, geopm_policy_affinity(policy, GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER));
    EXPECT_EQ(0, geopm_policy_goal(policy, GEOPM_FLAGS_GOAL_CPU_EFFICIENCY));
    EXPECT_EQ(0, geopm_policy_write(policy));
    EXPECT_EQ(0, geopm_policy_destroy(policy));
    EXPECT_EQ(0, remove(path));
}

TEST_F(GlobalPolicyTest, negative_c_interface)
{
    struct geopm_policy_c *policy = NULL;

    EXPECT_EQ(GEOPM_ERROR_INVALID, geopm_policy_create("", "", &policy));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_power(policy, 2500));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_mode(policy, GEOPM_MODE_FREQ_HYBRID_DYNAMIC));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_cpu_freq(policy, 2200));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_full_perf(policy, 8));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_tdp_percent(policy, 60));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_affinity(policy, GEOPM_FLAGS_SMALL_CPU_TOPOLOGY_SCATTER));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_goal(policy, GEOPM_FLAGS_GOAL_CPU_EFFICIENCY));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_write(policy));
    EXPECT_EQ(GEOPM_ERROR_POLICY_NULL, geopm_policy_destroy(policy));
}
