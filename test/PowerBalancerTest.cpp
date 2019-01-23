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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "CircularBuffer.hpp"
#include "PowerBalancer.hpp"
#include "Helper.hpp"

using geopm::PowerBalancer;
using ::testing::_;
using ::testing::Return;
using ::testing::Sequence;

class PowerBalancerTest : public ::testing::Test
{
    public:
        void SetUp(void);

    protected:
        double calc_rt(void);
        double calc_rt(double power_limit);

        const double M_CONTROL_LATENCY = 0.045;
        const double M_POWER_CAP = 300;
        const double M_TRIAL_DELTA = 1.0;
        const size_t M_NUM_SAMPLE = 3;
        const int M_MEASURE_DURATION = 0.05;
        std::unique_ptr<PowerBalancer> m_balancer;
};

void PowerBalancerTest::SetUp(void)
{
    m_balancer = geopm::make_unique<PowerBalancer>(M_CONTROL_LATENCY, M_TRIAL_DELTA, M_NUM_SAMPLE, M_MEASURE_DURATION);
    m_balancer->power_cap(M_POWER_CAP);
}

double PowerBalancerTest::calc_rt(void)
{
    return calc_rt(m_balancer->power_limit());
}

double PowerBalancerTest::calc_rt(double power_limit)
{
    return 1 / power_limit * 1e3;
}


TEST_F(PowerBalancerTest, power_cap)
{
    const double cap = 999;
    m_balancer->power_cap(cap);
    EXPECT_EQ(cap, m_balancer->power_cap());
    EXPECT_EQ(cap, m_balancer->power_limit());
}

TEST_F(PowerBalancerTest, is_runtime_stable)
{
    size_t count = 1;
    while (!m_balancer->is_runtime_stable(calc_rt())) {
        ++count;
    }
    EXPECT_EQ(M_NUM_SAMPLE, count);
}

TEST_F(PowerBalancerTest, balance)
{
    while (!m_balancer->is_runtime_stable(calc_rt())) {
    }

    const std::vector<double> power_targets = {280, 265.45};
    const std::vector<double> exp_step = {3, 6};
    const std::vector<double> exp_limit = {292, 276};
    const std::vector<double> ach_limit = {M_POWER_CAP + 5.0, 260};
    const std::vector<double> exp_limit2 = {300, 276};
    const std::vector<double> exp_slack = {8, 24};
    const std::vector<std::vector<double> > exp_sample = {{3.42466, 3.42466, 3.52113,
                                                           3.52113, 3.62319, 3.62319},
                                                          {3.33333, 3.42466, 3.42466,
                                                           3.52113, 3.52113, 3.62319,
                                                           3.62319, 3.73134, 3.73134}};
    ASSERT_EQ(power_targets.size(), exp_limit.size());
    ASSERT_EQ(exp_step.size(), exp_limit.size());
    ASSERT_EQ(exp_sample.size(), exp_limit.size());
    for (size_t x = 0; x < power_targets.size(); ++x) {
        const auto &sample_set = exp_sample[x];
        auto curr_exp_sample = sample_set.cbegin();
        const double target_power = power_targets[x];
        const double target_runtime = calc_rt(target_power);
        m_balancer->target_runtime(target_runtime);

        bool is_target_met = false;
        int num_step = 0;
        while (!is_target_met) {
            const double curr_rt = calc_rt();
            is_target_met = m_balancer->is_target_met(curr_rt);
            ++num_step;
            if (!is_target_met) {
                while (!m_balancer->is_runtime_stable(calc_rt())) {
                    m_balancer->calculate_runtime_sample();
                    EXPECT_NEAR(*curr_exp_sample, m_balancer->runtime_sample(), 1e-5);
                    curr_exp_sample++;
                }
            }
        }

        EXPECT_EQ(exp_step[x], num_step);
        EXPECT_EQ(exp_limit[x], m_balancer->power_limit());
        EXPECT_EQ(exp_slack[x], m_balancer->power_slack());
        EXPECT_GT(m_balancer->power_limit(), target_power);
        EXPECT_LT(m_balancer->power_limit(), M_POWER_CAP + M_TRIAL_DELTA);

        m_balancer->power_limit_adjusted(ach_limit[x]);
        EXPECT_EQ(ach_limit[x], m_balancer->power_limit());

        if (x < power_targets.size() - 1) {
            SetUp();
        }
    }
}
