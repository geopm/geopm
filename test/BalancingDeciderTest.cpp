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
#include "DeciderFactory.hpp"
#include "BalancingDecider.hpp"

class BalancingDeciderTest: public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        geopm::IDecider *m_balancer;
        geopm::PolicyFlags *m_flags;
        geopm::Policy *m_policy;
        geopm::Region *m_region;
        geopm::DeciderFactory *m_fact;
        struct geopm_policy_message_s m_policy_message;
        const int m_num_domain = 8;
};

void BalancingDeciderTest::SetUp()
{
    setenv("GEOPM_PLUGIN_PATH", ".libs/", 1);
    m_fact = new geopm::DeciderFactory();
    m_balancer = NULL;
    m_flags = NULL;
    m_policy = NULL;
    m_region = NULL;
    m_balancer = m_fact->decider("power_balancing");
    m_flags = new geopm::PolicyFlags(0);
    m_policy = new geopm::Policy(m_num_domain);
    m_region = new geopm::Region(GEOPM_REGION_ID_EPOCH, m_num_domain, 1);

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

    std::vector<struct geopm_sample_message_s> sample(8);

    for (int sample_idx = 0; sample_idx < 8; ++sample_idx) {
        sample[sample_idx].region_id = 42;
    }
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < GEOPM_NUM_SAMPLE_TYPE; j++) {
            for (int k = 0; k < 8; ++k) {
                sample[k].signal[j] = (double)(i + 1 + k);
            }
        }
        m_region->insert(sample);
    }
}

void BalancingDeciderTest::TearDown()
{
    if (m_policy) {
        delete m_policy;
    }
    if (m_flags) {
        delete m_flags;
    }
    if (m_region) {
        delete m_region;
    }
    if (m_balancer) {
        delete m_balancer;
    }
    if (m_fact) {
        delete m_fact;
    }
}

TEST_F(BalancingDeciderTest, name)
{
    EXPECT_TRUE(std::string("power_balancing") == m_balancer->name());
}

TEST_F(BalancingDeciderTest, clone)
{
    geopm::IDecider *cloned = m_balancer->clone();
    EXPECT_TRUE(std::string("power_balancing") == cloned->name());
    delete cloned;
}

TEST_F(BalancingDeciderTest, supported)
{
    EXPECT_TRUE(m_balancer->decider_supported(std::string("power_balancing")));
}

TEST_F(BalancingDeciderTest, new_policy_message)
{
    std::vector<double> tgt(m_num_domain);
    m_balancer->update_policy(m_policy_message, *m_policy);
    m_policy->target(GEOPM_REGION_ID_EPOCH, tgt);
    // The first time it should split evenly
    for (int dom = 0; dom < m_num_domain; ++dom) {
        EXPECT_DOUBLE_EQ(13, tgt[dom]);
    }
    // Now skew the power balance
    m_policy->update(GEOPM_REGION_ID_EPOCH, 0, 12.0);
    m_policy->update(GEOPM_REGION_ID_EPOCH, 1, 14.0);
    // Double the power budget
    m_policy_message.power_budget = 208;
    m_balancer->update_policy(m_policy_message, *m_policy);
    m_policy->target(GEOPM_REGION_ID_EPOCH, tgt);
    // The first time it should split evenly
    for (int dom = 0; dom < m_num_domain; ++dom) {
        if (dom == 0) {
            EXPECT_DOUBLE_EQ(24, tgt[dom]);
        }
        else if (dom == 1) {
            EXPECT_DOUBLE_EQ(28, tgt[dom]);
        }
        else {
            EXPECT_DOUBLE_EQ(26, tgt[dom]);
        }
    }
}

TEST_F(BalancingDeciderTest, update_policy)
{
    std::vector<double> tgt(m_num_domain);
    m_policy_message.power_budget = 800;
    m_balancer->update_policy(m_policy_message, *m_policy);
    m_policy->target(GEOPM_REGION_ID_EPOCH, tgt);
    // The first time it should split evenly
    for (int dom = 0; dom < m_num_domain; ++dom) {
        EXPECT_DOUBLE_EQ(100.0, tgt[dom]);
    }
    m_balancer->update_policy(*m_region, *m_policy);
    m_policy->target(GEOPM_REGION_ID_EPOCH, tgt);
    double expect = 89.705882352941174;
    for (int dom = 0; dom < m_num_domain; ++dom) {
        EXPECT_NEAR(expect + (dom * 2.94117647058825), tgt[dom], 1E-9);
    }
}
