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

#include <stdlib.h>
#include <iostream>
#include <map>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "DeciderFactory.hpp"
#include "GoverningDecider.hpp"
#include "Region.hpp"
#include "Policy.hpp"


class GoverningDeciderTest: public :: testing :: Test
{
    protected:
    void SetUp();
    void TearDown();
    void run_param_case(double budget, double pkg_power, double dram_power, int num_sockets);
    geopm::IDecider *m_decider;
    geopm::DeciderFactory *m_fact;
};

void GoverningDeciderTest::SetUp()
{
    setenv("GEOPM_PLUGIN_PATH", ".libs/", 1);
    m_fact = new geopm::DeciderFactory();
    m_decider = NULL;
    m_decider = m_fact->decider("power_governing");
}

void GoverningDeciderTest::TearDown()
{
    if (m_decider) {
        delete m_decider;
    }
    if (m_fact) {
        delete m_fact;
    }
}

/// @todo: Add test where domains have imbalanced power consumption.

TEST_F(GoverningDeciderTest, decider_is_supported)
{
    EXPECT_TRUE(m_decider->decider_supported("power_governing"));
    EXPECT_FALSE(m_decider->decider_supported("bad_string"));
}

TEST_F(GoverningDeciderTest, name)
{
    EXPECT_TRUE(std::string("power_governing") == m_decider->name());
}

TEST_F(GoverningDeciderTest, clone)
{
    geopm::IDecider *cloned = m_decider->clone();
    EXPECT_TRUE(std::string("power_governing") == cloned->name());
    delete cloned;
}

TEST_F(GoverningDeciderTest, 1_socket_under_budget)
{
    run_param_case(165.0, 125.0, 22.0, 1);
}

TEST_F(GoverningDeciderTest, 1_socket_over_budget)
{
    run_param_case(165.0, 155.0, 22.0, 1);
}

TEST_F(GoverningDeciderTest, 2_socket_under_budget)
{
    run_param_case(165.0, 120.0, 40.0, 2);
}

TEST_F(GoverningDeciderTest, 2_socket_over_budget)
{
    run_param_case(165.0, 150.0, 40.0, 2);
}

void GoverningDeciderTest::run_param_case(double budget, double pkg_power, double dram_power, int num_domain)
{
    const int region_id = 1;
    geopm::Region region(region_id, num_domain, 0, NULL);
    geopm::Policy policy(num_domain);

    struct geopm_policy_message_s policy_msg = {GEOPM_POLICY_MODE_DYNAMIC, 0, 1, budget};
    m_decider->update_policy(policy_msg, policy);
    std::vector<double> target(num_domain);
    policy.target(region_id, target);
    for (int i = 0; i < num_domain; ++i) {
        EXPECT_DOUBLE_EQ((budget / num_domain), target[0]);
    }

    std::vector<struct geopm_telemetry_message_s> telemetry(num_domain);
    for (int i = 0; i < num_domain; ++i) {
        telemetry[i].region_id = region_id;
        telemetry[i].timestamp = {{0, 0}};
    }
    int num_samples = 5;

    for (int i = 0; i < num_domain; ++i) {
        for (int j = 0; j < GEOPM_NUM_TELEMETRY_TYPE; ++j) {
            telemetry[i].signal[j] = 0.0;
        }
    }
    // Make power usage 147 Watts
    for (int i = 0; i < num_samples; ++i) {
        for (int j = 0; j < num_domain; ++j) {
            telemetry[j].timestamp.t.tv_sec = i;
            telemetry[j].signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] += (pkg_power / num_domain);
            telemetry[j].signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY] += (dram_power / num_domain);
        }
        region.insert(telemetry);
    }
    EXPECT_EQ(5, region.num_sample(0, GEOPM_TELEMETRY_TYPE_PKG_ENERGY));

    EXPECT_TRUE(m_decider->update_policy(region, policy));
    EXPECT_FALSE(policy.is_converged(region_id));
    policy.target(region_id, target);
    for (int i = 0; i < num_domain; ++i) {
        EXPECT_DOUBLE_EQ(((budget - dram_power) / num_domain), target[i]);
    }

    int convergence_num = 5;
    for (int i = 0; i < convergence_num; ++i) {
        for (int j = 0; j < num_samples; ++j) {
            for (int k = 0; k < num_domain; ++k) {
                telemetry[k].timestamp.t.tv_sec += 1;
                telemetry[k].signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] += (pkg_power / num_domain);
                telemetry[k].signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY] += (dram_power / num_domain);
            }
            region.insert(telemetry);
        }
        m_decider->update_policy(region, policy);
    }
    EXPECT_TRUE(policy.is_converged(region_id));
}
