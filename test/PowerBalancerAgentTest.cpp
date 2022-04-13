/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "MockPowerBalancer.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockSampleAggregator.hpp"
#include "PowerBalancerAgent.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

#include "config.h"

using geopm::PowerBalancerAgent;
using geopm::PowerBalancer;
using geopm::PlatformTopo;
using ::testing::_;
using ::testing::SetArgReferee;
using ::testing::ContainerEq;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::InvokeWithoutArgs;
using ::testing::InSequence;
using ::testing::AtLeast;


class PowerBalancerAgentTest : public ::testing::Test
{
    protected:
        void SetUp();

        enum {
            M_SIGNAL_EPOCH_COUNT,
            M_SIGNAL_EPOCH_RUNTIME,
            M_SIGNAL_EPOCH_RUNTIME_NETWORK,
            M_SIGNAL_EPOCH_RUNTIME_IGNORE,
        };

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::shared_ptr<MockSampleAggregator> m_sample_agg;
        std::vector<std::shared_ptr<MockPowerBalancer> > m_power_bal;
        std::unique_ptr<PowerBalancerAgent> m_agent;

        const double M_POWER_PACKAGE_MIN = 50;
        const double M_POWER_PACKAGE_TDP = 300;
        const double M_POWER_PACKAGE_MAX = 325;
        const int M_NUM_PKGS = 2;
        const std::vector<int> M_FAN_IN = {2, 2};
        const double M_TIME_WINDOW = 0.015;
};

void PowerBalancerAgentTest::SetUp()
{
    m_sample_agg = std::make_shared<MockSampleAggregator>();
    for (int i = 0; i < M_NUM_PKGS; ++i) {
        m_power_bal.push_back(std::make_shared<MockPowerBalancer>());
    }

    ON_CALL(m_platform_io, read_signal("POWER_PACKAGE_TDP", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_POWER_PACKAGE_TDP));
    ON_CALL(m_platform_io, read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_POWER_PACKAGE_MIN));
    ON_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(M_POWER_PACKAGE_MAX));
    ON_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Return(M_POWER_PACKAGE_MAX/M_NUM_PKGS));
    ON_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(M_NUM_PKGS));

    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_TDP", _, _));
    std::vector<std::shared_ptr<PowerBalancer> >power_bal_base(m_power_bal.size());
    std::copy(m_power_bal.begin(), m_power_bal.end(), power_bal_base.begin());
    m_agent = geopm::make_unique<PowerBalancerAgent>(m_platform_io, m_platform_topo,
                                                     m_sample_agg,
                                                     power_bal_base,
                                                     M_POWER_PACKAGE_MIN,
                                                     M_POWER_PACKAGE_MAX);
}

TEST_F(PowerBalancerAgentTest, tree_root_agent)
{
    const bool IS_ROOT = true;
    int level = 2;
    int num_children = M_FAN_IN[level - 1];

    ON_CALL(m_platform_io, read_signal("POWER_PACKAGE_MIN", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(50));
    ON_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(200));

    m_agent->init(level, M_FAN_IN, IS_ROOT);

    std::vector<double> in_policy {NAN, NAN, NAN, NAN};
    std::vector<std::vector<double> > exp_out_policy;

    std::vector<std::vector<double> > in_sample;
    std::vector<double> exp_out_sample;

    std::vector<std::vector<double> > out_policy = std::vector<std::vector<double> >(num_children, {NAN, NAN, NAN, NAN});
    std::vector<double> out_sample = {NAN, NAN, NAN, NAN};

#ifdef GEOPM_DEBUG
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->adjust_platform(in_policy), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->sample_platform(out_sample), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
    std::vector<double> trace_data;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->trace_values(trace_data), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
#endif

    int ctl_step = 0;
    double curr_cap = 300;
    double curr_cnt = (double) ctl_step;
    double curr_epc = 0.0;
    double curr_slk = 0.0;
    double curr_hrm = 0.0;
    bool exp_descend_ret = true;
    bool exp_ascend_ret  = true;
    bool desc_ret;
    bool ascend_ret;
    /// M_STEP_SEND_DOWN_LIMIT
    {
    in_policy = {curr_cap, curr_cnt, curr_epc, curr_slk};
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {curr_cap, curr_cnt, curr_epc, curr_slk});
    in_sample = std::vector<std::vector<double> >(num_children, {(double)ctl_step, curr_epc, curr_slk, curr_hrm});
    exp_out_sample = {(double)ctl_step, curr_epc, curr_slk, curr_hrm};

#ifdef GEOPM_DEBUG
    std::vector<std::vector<double> > inv_out_policy = {};
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->split_policy({}, out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->split_policy(in_policy, inv_out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
#endif
    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

#ifdef GEOPM_DEBUG
    std::vector<double> inv_out_sample = {};
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->aggregate_sample({}, out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->aggregate_sample(in_sample, inv_out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
#endif
    m_agent->aggregate_sample(in_sample, out_sample);
    ascend_ret = m_agent->do_send_sample();
    EXPECT_EQ(exp_ascend_ret, ascend_ret);
    EXPECT_THAT(out_sample, ContainerEq(exp_out_sample));
    }

    ctl_step = 1;
    curr_cnt = (double) ctl_step;

    /// M_STEP_MEASURE_RUNTIME
    {
    in_policy = {curr_cap, 0.0, 0.0, 0.0};
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {0.0, curr_cnt, curr_epc, curr_slk});
    curr_epc = 22.0;
    in_sample = std::vector<std::vector<double> >(num_children, {(double)ctl_step, curr_epc, curr_slk, curr_hrm});
    exp_out_sample = {(double)ctl_step, curr_epc, curr_slk, curr_hrm};

    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    m_agent->aggregate_sample(in_sample, out_sample);
    ascend_ret = m_agent->do_send_sample();
    EXPECT_EQ(exp_ascend_ret, ascend_ret);
    EXPECT_THAT(out_sample, ContainerEq(exp_out_sample));
    }

    ctl_step = 2;
    curr_cnt = (double) ctl_step;

    /// M_STEP_REDUCE_LIMIT
    {
    in_policy = {curr_cap, 0.0, 0.0, 0.0};
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {0.0, curr_cnt, curr_epc, curr_slk});
    curr_slk = 9.0;
    in_sample = std::vector<std::vector<double> >(num_children, {(double)ctl_step, curr_epc, curr_slk, curr_hrm});
    exp_out_sample = {(double)ctl_step, curr_epc, num_children * curr_slk, curr_hrm};///@todo update when/if updated to use unique child sample inputs

    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    m_agent->aggregate_sample(in_sample, out_sample);
    ascend_ret = m_agent->do_send_sample();
    EXPECT_EQ(exp_ascend_ret, ascend_ret);
    EXPECT_THAT(out_sample, ContainerEq(exp_out_sample));
    }

    ctl_step = 3;
    curr_cnt = (double) ctl_step;
    curr_slk = 0.0;///@todo
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {0.0, curr_cnt, curr_epc, curr_slk});

    /// M_STEP_SEND_DOWN_LIMIT
    {
    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));
    }
}

TEST_F(PowerBalancerAgentTest, tree_agent)
{
    const bool IS_ROOT = false;
    int level = 1;
    int num_children = M_FAN_IN[level - 1];

    m_agent->init(level, M_FAN_IN, IS_ROOT);

    std::vector<double> in_policy {NAN, NAN, NAN, NAN};
    std::vector<std::vector<double> > exp_out_policy;

    std::vector<std::vector<double> > in_sample;
    std::vector<double> exp_out_sample;

    std::vector<std::vector<double> > out_policy = std::vector<std::vector<double> >(num_children, {NAN, NAN, NAN, NAN});
    std::vector<double> out_sample = {NAN, NAN, NAN, NAN};

#ifdef GEOPM_DEBUG
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->adjust_platform(in_policy), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->sample_platform(out_sample), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
    std::vector<double> trace_data;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->trace_values(trace_data), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
#endif

    int ctl_step = 0;
    double curr_cap = 300;
    double curr_cnt = (double) ctl_step;
    double curr_epc = 0.0;
    double curr_slk = 0.0;
    double curr_hrm = 0.0;
    bool exp_descend_ret = true;
    bool exp_ascend_ret  = true;
    bool desc_ret;
    bool ascend_ret;
    /// M_STEP_SEND_DOWN_LIMIT
    {
    in_policy = {curr_cap, curr_cnt, curr_epc, curr_slk};
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {curr_cap, curr_cnt, curr_epc, curr_slk});
    in_sample = std::vector<std::vector<double> >(num_children, {(double)ctl_step, curr_epc, curr_slk, curr_hrm});
    exp_out_sample = {(double)ctl_step, curr_epc, 0.0, 0.0};

#ifdef GEOPM_DEBUG
    std::vector<std::vector<double> > inv_out_policy = {};
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->split_policy({}, out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->split_policy(in_policy, inv_out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
#endif
    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

#ifdef GEOPM_DEBUG
    std::vector<double> inv_out_sample = {};
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->aggregate_sample({}, out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->aggregate_sample(in_sample, inv_out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
#endif
    m_agent->aggregate_sample(in_sample, out_sample);
    ascend_ret = m_agent->do_send_sample();
    EXPECT_EQ(exp_ascend_ret, ascend_ret);
    EXPECT_THAT(out_sample, ContainerEq(exp_out_sample));
    }

    ctl_step = 1;
    curr_cnt = (double) ctl_step;

    /// M_STEP_MEASURE_RUNTIME
    {
    in_policy = {0.0, curr_cnt, 0.0, 0.0};
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {0.0, curr_cnt, curr_epc, curr_slk});
    curr_epc = 22.0;
    in_sample = std::vector<std::vector<double> >(num_children, {(double)ctl_step, curr_epc, curr_slk, curr_hrm});
    exp_out_sample = {(double)ctl_step, curr_epc, curr_slk, curr_hrm};

    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    m_agent->aggregate_sample(in_sample, out_sample);
    ascend_ret = m_agent->do_send_sample();
    EXPECT_EQ(exp_ascend_ret, ascend_ret);
    EXPECT_THAT(out_sample, ContainerEq(exp_out_sample));
    }

    ctl_step = 2;
    curr_cnt = (double) ctl_step;

    /// M_STEP_REDUCE_LIMIT
    {
    in_policy = {0.0, curr_cnt, curr_epc, 0.0};
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {0.0, curr_cnt, curr_epc, curr_slk});
    curr_slk = 9.0;
    in_sample = std::vector<std::vector<double> >(num_children, {(double)ctl_step, curr_epc, curr_slk, curr_hrm});
    exp_out_sample = {(double)ctl_step, curr_epc, num_children * curr_slk, curr_hrm};///@todo update when/if updated to use unique child sample inputs

    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    m_agent->aggregate_sample(in_sample, out_sample);
    ascend_ret = m_agent->do_send_sample();
    EXPECT_EQ(exp_ascend_ret, ascend_ret);
    EXPECT_THAT(out_sample, ContainerEq(exp_out_sample));
    }

    ctl_step = 3;
    curr_cnt = (double) ctl_step;
    curr_slk /= num_children;
    exp_out_policy = std::vector<std::vector<double> >(num_children,
                                                       {0.0, curr_cnt, 0.0, curr_slk});

    /// M_STEP_SEND_DOWN_LIMIT
    {
    in_policy = {0.0, curr_cnt, 0.0, curr_slk};
    m_agent->split_policy(in_policy, out_policy);
    desc_ret = m_agent->do_send_policy();
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));
    }
}

TEST_F(PowerBalancerAgentTest, enforce_policy)
{
    const double limit = 100;
    const std::vector<double> policy{limit, NAN, NAN, NAN};
    const std::vector<double> bad_policy{100};

    EXPECT_CALL(m_platform_io, control_domain_type("POWER_PACKAGE_LIMIT"))
        .WillOnce(Return(GEOPM_DOMAIN_PACKAGE));
    EXPECT_CALL(m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillOnce(Return(M_NUM_PKGS));
    EXPECT_CALL(m_platform_io, write_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_BOARD,
                                             0, limit/M_NUM_PKGS));

    m_agent->enforce_policy(policy);

    EXPECT_THROW(m_agent->enforce_policy(bad_policy), geopm::Exception);
}

TEST_F(PowerBalancerAgentTest, validate_policy)
{
    std::vector<double> policy;

    // valid policy unchanged
    policy = {100};
    m_agent->validate_policy(policy);
    EXPECT_EQ(100, policy[0]);

    // NAN becomes default
    policy = {NAN};
    m_agent->validate_policy(policy);
    EXPECT_EQ(M_POWER_PACKAGE_TDP, policy[0]);

    // clamp to min
    policy = {M_POWER_PACKAGE_MIN - 1};
    m_agent->validate_policy(policy);
    EXPECT_EQ(M_POWER_PACKAGE_MIN, policy[0]);

    // clamp to max
    policy = {M_POWER_PACKAGE_MAX + 1};
    m_agent->validate_policy(policy);
    EXPECT_EQ(M_POWER_PACKAGE_MAX, policy[0]);

}
