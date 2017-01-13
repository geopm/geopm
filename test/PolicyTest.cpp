/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <iostream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "Policy.hpp"

class PolicyTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        geopm::IPolicy *m_policy;
        geopm::PolicyFlags* m_flags;
        struct geopm_policy_message_s m_policy_message;
        const int m_num_domain = 8;
};

void PolicyTest::SetUp()
{
    m_policy = new geopm::Policy(m_num_domain);
    m_flags = new geopm::PolicyFlags(0);

    std::vector<double> target(m_num_domain);
    std::fill(target.begin(), target.end(), 13.0);
    m_policy->update((uint64_t)13, GEOPM_CONTROL_TYPE_POWER, target);
    std::fill(target.begin(), target.end(), 21.0);
    m_policy->update((uint64_t)21, GEOPM_CONTROL_TYPE_POWER, target);
    for (int i = 0; i < m_num_domain; i++) {
        if (i == m_num_domain / 2) {
            m_policy->update((uint64_t)42, GEOPM_CONTROL_TYPE_POWER, i, -DBL_MAX);
        }
        else {
            m_policy->update((uint64_t)42, GEOPM_CONTROL_TYPE_POWER, i, 42.0);
        }
    }
    m_flags->frequency_mhz(1200);
    m_flags->tdp_percent(90);
    m_flags->affinity(GEOPM_POLICY_AFFINITY_COMPACT);
    m_flags->goal(GEOPM_POLICY_GOAL_CPU_EFFICIENCY);
    m_flags->num_max_perf(4);
    m_policy->mode(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC);
    m_policy->policy_flags(m_flags->flags());

    m_policy_message.mode = GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC;
    m_policy_message.flags = m_flags->flags();
    m_policy_message.num_sample = 8;
    m_policy_message.power_budget = 104;
}

void PolicyTest::TearDown()
{
    delete m_policy;
    delete m_flags;
}

TEST_F(PolicyTest, converged)
{
    EXPECT_FALSE(m_policy->is_converged(21));
    m_policy->is_converged(21, true);
    EXPECT_TRUE(m_policy->is_converged(21));
}

TEST_F(PolicyTest, num_domain)
{
    EXPECT_EQ(m_num_domain, m_policy->num_domain());
}

TEST_F(PolicyTest, region_id)
{
    std::vector<uint64_t> id;

    m_policy->region_id(id);
    // Need to take into account the EPOCH region
    EXPECT_EQ((size_t)4, id.size());
    EXPECT_EQ((uint64_t)13, id[0]);
    EXPECT_EQ((uint64_t)21, id[1]);
    EXPECT_EQ((uint64_t)42, id[2]);
    EXPECT_EQ((uint64_t)GEOPM_REGION_ID_EPOCH, id[3]);
}

TEST_F(PolicyTest, mode)
{
    EXPECT_EQ(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC, m_policy->mode());
}

TEST_F(PolicyTest, frequency)
{
    EXPECT_EQ(1200, m_policy->frequency_mhz());
}

TEST_F(PolicyTest, tdp_percent)
{
    EXPECT_EQ(90, m_policy->tdp_percent());
}

TEST_F(PolicyTest, affinity)
{
    EXPECT_EQ(GEOPM_POLICY_AFFINITY_COMPACT, m_policy->affinity());
}

TEST_F(PolicyTest, goal)
{
    EXPECT_EQ(GEOPM_POLICY_GOAL_CPU_EFFICIENCY, m_policy->goal());
}

TEST_F(PolicyTest, num_max_perf)
{
    EXPECT_EQ(4, m_policy->num_max_perf());
}


TEST_F(PolicyTest, target)
{
    std::vector<double> tgt(m_num_domain);
    double actual;

    m_policy->target(13, GEOPM_CONTROL_TYPE_POWER, tgt);
    for (int i = 0; i < m_num_domain; ++i) {
        EXPECT_DOUBLE_EQ(13.0, tgt[i]);
    }
    m_policy->target(21, GEOPM_CONTROL_TYPE_POWER, tgt);
    for (int i = 0; i < m_num_domain; ++i) {
        EXPECT_DOUBLE_EQ(21.0, tgt[i]);
    }
    for (int i = 0; i < m_num_domain; ++i) {
        m_policy->target(42, GEOPM_CONTROL_TYPE_POWER, i, actual);
        if (i == m_num_domain / 2) {
            EXPECT_DOUBLE_EQ(-DBL_MAX, actual);
        }
        else {
            EXPECT_DOUBLE_EQ(42.0, actual);
        }
    }
}

TEST_F(PolicyTest, policy_message)
{
    std::vector<struct geopm_policy_message_s> child_msg(m_num_domain);

    m_policy->policy_message(13, m_policy_message, child_msg);
    for (int i = 0; i < m_num_domain; ++i) {
        EXPECT_EQ(GEOPM_POLICY_MODE_PERF_BALANCE_DYNAMIC, child_msg[i].mode);
        EXPECT_EQ(m_flags->flags(),  child_msg[i].flags);
        EXPECT_EQ(8, child_msg[i].num_sample);
        EXPECT_DOUBLE_EQ(13.0, child_msg[i].power_budget);
    }
}

TEST_F(PolicyTest, negative_unsized_vector)
{
    std::vector<struct geopm_policy_message_s> child_msg;
    std::vector<double> target;

    int thrown = 0;
    try {
        m_policy->update(13, GEOPM_CONTROL_TYPE_POWER, target);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_policy->target(13, GEOPM_CONTROL_TYPE_POWER, target);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_policy->policy_message(13, m_policy_message, child_msg);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
}

TEST_F(PolicyTest, negative_index_oob)
{
    int thrown = 0;
    double target = 13.0;
    try {
        m_policy->update(13, GEOPM_CONTROL_TYPE_POWER, m_num_domain + 1, target);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
    thrown = 0;
    try {
        m_policy->target(13, GEOPM_CONTROL_TYPE_POWER, m_num_domain + 1, target);
    }
    catch (geopm::Exception e) {
        thrown = e.err_value();
    }
    EXPECT_EQ(GEOPM_ERROR_INVALID, thrown);
}
