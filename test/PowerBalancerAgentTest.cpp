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

#include "MockPowerGovernor.hpp"
#include "MockPowerBalancer.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "PowerBalancerAgent.hpp"
#include "Helper.hpp"
#include "geopm_test.hpp"

#include "config.h"

using geopm::PowerBalancerAgent;
using geopm::IPlatformTopo;
using ::testing::_;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::ContainerEq;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::InvokeWithoutArgs;
using ::testing::InSequence;

class PowerBalancerAgentTest : public ::testing::Test
{
    protected:
        enum {
            M_SIGNAL_EPOCH_COUNT,
            M_SIGNAL_EPOCH_RUNTIME,
            M_SIGNAL_EPOCH_RUNTIME_MPI,
            M_SIGNAL_EPOCH_RUNTIME_IGNORE,
        };

        MockPlatformIO m_platform_io;
        MockPlatformTopo m_platform_topo;
        std::unique_ptr<MockPowerGovernor> m_power_gov;
        std::unique_ptr<MockPowerBalancer> m_power_bal;
        std::unique_ptr<PowerBalancerAgent> m_agent;

        const double M_POWER_PACKAGE_MAX = 325;
        const int M_NUM_PKGS = 1;
        const std::vector<int> M_FAN_IN = {2, 2};
};

TEST_F(PowerBalancerAgentTest, power_balancer_agent)
{
    const std::vector<std::string> exp_pol_names = {"POWER_CAP",
                                                    "STEP_COUNT",
                                                    "MAX_EPOCH_RUNTIME",
                                                    "POWER_SLACK"};
    const std::vector<std::string> exp_smp_names = {"STEP_COUNT",
                                                    "MAX_EPOCH_RUNTIME",
                                                    "SUM_POWER_SLACK",
                                                    "MIN_POWER_HEADROOM"};
    m_agent = geopm::make_unique<PowerBalancerAgent>(m_platform_io, m_platform_topo,
                                                     std::move(m_power_gov), std::move(m_power_bal));

    PowerBalancerAgent::make_plugin();
    EXPECT_EQ("power_balancer", m_agent->plugin_name());
    EXPECT_EQ(exp_pol_names, m_agent->policy_names());
    EXPECT_EQ(exp_smp_names, m_agent->sample_names());
    m_agent->report_header();
    m_agent->report_node();
    m_agent->report_region();
    m_agent->wait();

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->init(0, {}, false), GEOPM_ERROR_RUNTIME, "single node job detected, user power_governor.");
}

TEST_F(PowerBalancerAgentTest, tree_root_agent)
{
    const bool IS_ROOT = true;
    int level = 2;
    int num_children = M_FAN_IN[level - 1];

    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MIN", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(50));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(200));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_TDP", IPlatformTopo::M_DOMAIN_PACKAGE, 0))
        .WillOnce(Return(150));
    EXPECT_CALL(m_platform_io, control_domain_type("POWER_PACKAGE_LIMIT"))
        .WillOnce(Return(IPlatformTopo::M_DOMAIN_PACKAGE));
    EXPECT_CALL(m_platform_topo, num_domain(IPlatformTopo::M_DOMAIN_PACKAGE))
        .WillOnce(Return(2));

    m_agent = geopm::make_unique<PowerBalancerAgent>(m_platform_io, m_platform_topo,
                                                     std::move(m_power_gov), std::move(m_power_bal));
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
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->trace_names(), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
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
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->descend({}, out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->descend(in_policy, inv_out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
#endif
    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

#ifdef GEOPM_DEBUG
    std::vector<double> inv_out_sample = {};
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->ascend({}, out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->ascend(in_sample, inv_out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
#endif
    ascend_ret = m_agent->ascend(in_sample, out_sample);
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

    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    ascend_ret = m_agent->ascend(in_sample, out_sample);
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

    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    ascend_ret = m_agent->ascend(in_sample, out_sample);
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
    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));
    }
}

TEST_F(PowerBalancerAgentTest, tree_agent)
{
    const bool IS_ROOT = false;
    int level = 1;
    int num_children = M_FAN_IN[level - 1];

    m_agent = geopm::make_unique<PowerBalancerAgent>(m_platform_io, m_platform_topo,
                                                     std::move(m_power_gov), std::move(m_power_bal));
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
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->trace_names(), GEOPM_ERROR_LOGIC, "was called on non-leaf agent");
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
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->descend({}, out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->descend(in_policy, inv_out_policy), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
#endif
    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

#ifdef GEOPM_DEBUG
    std::vector<double> inv_out_sample = {};
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->ascend({}, out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->ascend(in_sample, inv_out_sample), GEOPM_ERROR_LOGIC, "sample vectors not correctly sized.");
#endif
    ascend_ret = m_agent->ascend(in_sample, out_sample);
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

    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    ascend_ret = m_agent->ascend(in_sample, out_sample);
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

    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));

    ascend_ret = m_agent->ascend(in_sample, out_sample);
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
    desc_ret = m_agent->descend(in_policy, out_policy);
    EXPECT_EQ(exp_descend_ret, desc_ret);
    EXPECT_THAT(out_policy, ContainerEq(exp_out_policy));
    }
}

TEST_F(PowerBalancerAgentTest, leaf_agent)
{
    const bool IS_ROOT = false;
    int level = 0;
    int num_children = 1;
    int counter = 0;
    std::vector<double> trace_vals(7, NAN);
    std::vector<double> exp_trace_vals(7, NAN);
    const std::vector<std::string> trace_cols = {"epoch_runtime",
                                                 "power_limit",
                                                 "policy_power_cap",
                                                 "policy_step_count",
                                                 "policy_max_epoch_runtime",
                                                 "policy_power_slack",
                                                 "policy_power_limit"};
    std::vector<double> epoch_rt_mpi = {0.50, 0.75};
    std::vector<double> epoch_rt_ignore = {0.25, 0.27};
    std::vector<double> epoch_rt = {1.0, 1.01};

    EXPECT_CALL(m_platform_topo, num_domain(IPlatformTopo::M_DOMAIN_PACKAGE))
        .WillOnce(Return(M_NUM_PKGS));
    EXPECT_CALL(m_platform_io, read_signal("POWER_PACKAGE_MAX", IPlatformTopo::M_DOMAIN_PACKAGE, _))
        .WillOnce(Return(M_POWER_PACKAGE_MAX));
    EXPECT_CALL(m_platform_io, push_signal("EPOCH_COUNT", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_SIGNAL_EPOCH_COUNT));
    EXPECT_CALL(m_platform_io, push_signal("EPOCH_RUNTIME", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_SIGNAL_EPOCH_RUNTIME));
    EXPECT_CALL(m_platform_io, sample(M_SIGNAL_EPOCH_COUNT))
        .WillRepeatedly(InvokeWithoutArgs([&counter]()
                {
                    return (double) ++counter;
                }));
    EXPECT_CALL(m_platform_io, push_signal("EPOCH_RUNTIME_MPI", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_SIGNAL_EPOCH_RUNTIME_MPI));
    EXPECT_CALL(m_platform_io, push_signal("EPOCH_RUNTIME_IGNORE", IPlatformTopo::M_DOMAIN_BOARD, 0))
        .WillOnce(Return(M_SIGNAL_EPOCH_RUNTIME_IGNORE));

    Sequence e_rt_pio_seq;
    for (size_t x = 0; x < epoch_rt.size(); ++x) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_EPOCH_RUNTIME))
            .InSequence(e_rt_pio_seq)
            .WillOnce(Return(epoch_rt[x]));
    }
    Sequence mpi_rt_pio_seq;
    for (size_t x = 0; x < epoch_rt_mpi.size(); ++x) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_EPOCH_RUNTIME_MPI))
            .InSequence(mpi_rt_pio_seq)
            .WillOnce(Return(epoch_rt_mpi[x]));
    }
    Sequence i_rt_pio_seq;
    for (size_t x = 0; x < epoch_rt_ignore.size(); ++x) {
        EXPECT_CALL(m_platform_io, sample(M_SIGNAL_EPOCH_RUNTIME_IGNORE))
            .InSequence(i_rt_pio_seq)
            .WillOnce(Return(epoch_rt_ignore[x]));
    }

    m_power_gov = geopm::make_unique<MockPowerGovernor>();
    EXPECT_CALL(*m_power_gov, init_platform_io());
    EXPECT_CALL(*m_power_gov, sample_platform())
        .Times(4);
    double actual_limit = 299.0;
    EXPECT_CALL(*m_power_gov, adjust_platform(300.0, _))
        .Times(4)
        .WillRepeatedly(DoAll(SetArgReferee<1>(actual_limit), Return(true)));
    m_power_bal = geopm::make_unique<MockPowerBalancer>();
    EXPECT_CALL(*m_power_bal, power_limit_adjusted(actual_limit))
        .Times(4);

    EXPECT_CALL(*m_power_bal, target_runtime(epoch_rt[0]));
    EXPECT_CALL(*m_power_bal, calculate_runtime_sample()).Times(2);
    EXPECT_CALL(*m_power_bal, runtime_sample())
        .WillRepeatedly(Return(epoch_rt[0]));
    double exp_in = epoch_rt[0] - epoch_rt_mpi[0] - epoch_rt_ignore[0];
    EXPECT_CALL(*m_power_bal, is_runtime_stable(exp_in))
        .WillOnce(Return(true));
    exp_in = epoch_rt[1] - epoch_rt_mpi[1] - epoch_rt_ignore[1];
    EXPECT_CALL(*m_power_bal, is_target_met(exp_in))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*m_power_bal, power_cap(300.0))
        .Times(2);
    EXPECT_CALL(*m_power_bal, power_cap())
        .WillRepeatedly(Return(300.0));
    EXPECT_CALL(*m_power_bal, power_limit())
        .WillRepeatedly(Return(300.0));
    m_agent = geopm::make_unique<PowerBalancerAgent>(m_platform_io, m_platform_topo,
                                                     std::move(m_power_gov), std::move(m_power_bal));
    m_agent->init(level, M_FAN_IN, IS_ROOT);

    EXPECT_EQ(trace_cols, m_agent->trace_names());
    std::vector<double> in_policy {NAN, NAN, NAN, NAN};

    std::vector<double> exp_out_sample;

    std::vector<std::vector<double> > out_policy = std::vector<std::vector<double> >(num_children, {NAN, NAN, NAN, NAN});
    std::vector<double> out_sample = {NAN, NAN, NAN, NAN};

#ifdef GEOPM_DEBUG
    std::vector<double> err_trace_vals, err_out_sample;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->trace_values(err_trace_vals), GEOPM_ERROR_LOGIC, "values vector not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->adjust_platform({}), GEOPM_ERROR_LOGIC, "policy vectors are not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->sample_platform(err_out_sample), GEOPM_ERROR_LOGIC, "out_sample vector not correctly sized.");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->descend(in_policy, out_policy), GEOPM_ERROR_LOGIC, "was called on non-tree agent");
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->ascend({}, out_sample), GEOPM_ERROR_LOGIC, "was called on non-tree agent");
#endif

    int ctl_step = 0;
    double curr_cap = 300;
    double curr_cnt = (double) ctl_step;
    double curr_epc = 0.0;
    double curr_slk = 0.0;
    bool exp_adj_plat_ret = true;
    bool exp_smp_plat_ret  = true;
    bool adj_ret;
    bool smp_ret;
    /// M_STEP_SEND_DOWN_LIMIT
    {
    in_policy = {curr_cap, curr_cnt, curr_epc, curr_slk};
    exp_out_sample = {(double)ctl_step, curr_epc, 0.0, 0.0};

    adj_ret = m_agent->adjust_platform(in_policy);
    EXPECT_EQ(exp_adj_plat_ret, adj_ret);

    smp_ret = m_agent->sample_platform(out_sample);
    EXPECT_EQ(exp_smp_plat_ret, smp_ret);
    EXPECT_EQ(out_sample, exp_out_sample);
    m_agent->trace_values(trace_vals);
    //EXPECT_EQ(exp_trace_vals, trace_vals);
    }

    ctl_step = 1;
    curr_cnt = (double) ctl_step;
    /// M_STEP_MEASURE_RUNTIME
    {
    in_policy = {0.0, curr_cnt, curr_epc, curr_slk};
    curr_epc = epoch_rt[ctl_step - 1];
    exp_out_sample = {(double)ctl_step, curr_epc, 0.0, 0.0};

    adj_ret = m_agent->adjust_platform(in_policy);
    EXPECT_EQ(exp_adj_plat_ret, adj_ret);

    smp_ret = m_agent->sample_platform(out_sample);
    EXPECT_EQ(exp_smp_plat_ret, smp_ret);
    EXPECT_EQ(out_sample, exp_out_sample);
    }

    ctl_step = 2;
    curr_cnt = (double) ctl_step;
    /// M_STEP_REDUCE_LIMIT
    {
    in_policy = {0.0, curr_cnt, curr_epc, curr_slk};
    exp_out_sample = {(double)ctl_step, curr_epc, 0.0, 25.0};

    adj_ret = m_agent->adjust_platform(in_policy);
    EXPECT_EQ(exp_adj_plat_ret, adj_ret);

    smp_ret = m_agent->sample_platform(out_sample);
    EXPECT_EQ(exp_smp_plat_ret, smp_ret);
    EXPECT_EQ(out_sample, exp_out_sample);
    }

    ctl_step = 3;
    curr_cnt = (double) ctl_step;
    /// M_STEP_SEND_DOWN_LIMIT
    {
    in_policy = {0.0, curr_cnt, curr_epc, curr_slk};
    exp_out_sample = {(double)ctl_step, curr_epc, 0.0, 25.0};

    adj_ret = m_agent->adjust_platform(in_policy);
    EXPECT_EQ(exp_adj_plat_ret, adj_ret);

    smp_ret = m_agent->sample_platform(out_sample);
    EXPECT_EQ(exp_smp_plat_ret, smp_ret);
    EXPECT_EQ(out_sample, exp_out_sample);
    }
}
