/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "PluginFactory.hpp"
#include "Agent.hpp"
#include "MonitorAgent.hpp"
#include "PowerBalancerAgent.hpp"
#include "PowerGovernorAgent.hpp"
#include "EnergyEfficientAgent.hpp"
#include "FrequencyMapAgent.hpp"

using geopm::Agent;

TEST(AgentFactoryTest, static_info_monitor)
{
    auto &factory = geopm::agent_factory();
    std::string agent_name = geopm::MonitorAgent::plugin_name();
    auto &dict = factory.dictionary(agent_name);
    EXPECT_EQ(0, Agent::num_policy(dict));
    EXPECT_EQ(0, Agent::num_sample(dict));
    EXPECT_EQ(0, Agent::num_policy(agent_name));
    EXPECT_EQ(0, Agent::num_sample(agent_name));
    std::vector<std::string> exp_sample = {};
    std::vector<std::string> exp_policy = {};
    EXPECT_EQ(exp_sample, Agent::sample_names(dict));
    EXPECT_EQ(exp_policy, Agent::policy_names(dict));

    EXPECT_EQ(exp_sample, Agent::sample_names(agent_name));
    EXPECT_EQ(exp_policy, Agent::policy_names(agent_name));
}

TEST(AgentFactoryTest, static_info_balancer)
{
    auto &factory = geopm::agent_factory();
    std::string agent_name = geopm::PowerBalancerAgent::plugin_name();
    auto &dict = factory.dictionary(agent_name);
    EXPECT_EQ(4, Agent::num_policy(dict));
    EXPECT_EQ(4, Agent::num_sample(dict));
    EXPECT_EQ(4, Agent::num_policy(agent_name));
    EXPECT_EQ(4, Agent::num_sample(agent_name));
    std::vector<std::string> exp_sample = {"STEP_COUNT",
                                           "MAX_EPOCH_RUNTIME",
                                           "SUM_POWER_SLACK",
                                           "MIN_POWER_HEADROOM"};
    std::vector<std::string> exp_policy = {"POWER_PACKAGE_LIMIT_TOTAL",
                                           "STEP_COUNT",
                                           "MAX_EPOCH_RUNTIME",
                                           "POWER_SLACK"};
    EXPECT_EQ(exp_sample, Agent::sample_names(dict));
    EXPECT_EQ(exp_policy, Agent::policy_names(dict));

    EXPECT_EQ(exp_sample, Agent::sample_names(agent_name));
    EXPECT_EQ(exp_policy, Agent::policy_names(agent_name));
}

TEST(AgentFactoryTest, static_info_governor)
{
    auto &factory = geopm::agent_factory();
    std::string agent_name = geopm::PowerGovernorAgent::plugin_name();
    auto &dict = factory.dictionary(agent_name);
    EXPECT_EQ(1, Agent::num_policy(dict));
    EXPECT_EQ(3, Agent::num_sample(dict));
    EXPECT_EQ(1, Agent::num_policy(agent_name));
    EXPECT_EQ(3, Agent::num_sample(agent_name));
    std::vector<std::string> exp_sample = {"POWER",
                                           "IS_CONVERGED",
                                           "POWER_AVERAGE_ENFORCED"};
    std::vector<std::string> exp_policy = {"POWER_PACKAGE_LIMIT_TOTAL"};
    EXPECT_EQ(exp_sample, Agent::sample_names(dict));
    EXPECT_EQ(exp_policy, Agent::policy_names(dict));

    EXPECT_EQ(exp_sample, Agent::sample_names(agent_name));
    EXPECT_EQ(exp_policy, Agent::policy_names(agent_name));
}

TEST(AgentFactoryTest, static_info_energy_efficient)
{
    auto &factory = geopm::agent_factory();
    std::string agent_name = geopm::EnergyEfficientAgent::plugin_name();
    auto &dict = factory.dictionary(agent_name);
    EXPECT_EQ(4, Agent::num_policy(dict));
    EXPECT_EQ(0, Agent::num_sample(dict));
    EXPECT_EQ(4, Agent::num_policy(agent_name));
    EXPECT_EQ(0, Agent::num_sample(agent_name));
    std::vector<std::string> exp_sample = {};
    std::vector<std::string> exp_policy = {"FREQ_MIN",
                                           "FREQ_MAX",
                                           "PERF_MARGIN",
                                           "FREQ_FIXED"};
    EXPECT_EQ(exp_sample, Agent::sample_names(dict));
    EXPECT_EQ(exp_policy, Agent::policy_names(dict));

    EXPECT_EQ(exp_sample, Agent::sample_names(agent_name));
    EXPECT_EQ(exp_policy, Agent::policy_names(agent_name));
}

TEST(AgentFactoryTest, static_info_frequency_map)
{
    auto &factory = geopm::agent_factory();
    std::string agent_name = geopm::FrequencyMapAgent::plugin_name();
    auto &dict = factory.dictionary(agent_name);
    EXPECT_EQ(64, Agent::num_policy(dict));
    EXPECT_EQ(0, Agent::num_sample(dict));
    EXPECT_EQ(64, Agent::num_policy(agent_name));
    EXPECT_EQ(0, Agent::num_sample(agent_name));
    std::vector<std::string> exp_sample = {};
    std::vector<std::string> exp_policy = {"FREQ_MIN",
                                           "FREQ_MAX",
                                           "HASH_0", "FREQ_0",
                                           "HASH_1", "FREQ_1",
                                           "HASH_2", "FREQ_2",
                                           "HASH_3", "FREQ_3",
                                           "HASH_4", "FREQ_4",
                                           "HASH_5", "FREQ_5",
                                           "HASH_6", "FREQ_6",
                                           "HASH_7", "FREQ_7",
                                           "HASH_8", "FREQ_8",
                                           "HASH_9", "FREQ_9",
                                           "HASH_10", "FREQ_10",
                                           "HASH_11", "FREQ_11",
                                           "HASH_12", "FREQ_12",
                                           "HASH_13", "FREQ_13",
                                           "HASH_14", "FREQ_14",
                                           "HASH_15", "FREQ_15",
                                           "HASH_16", "FREQ_16",
                                           "HASH_17", "FREQ_17",
                                           "HASH_18", "FREQ_18",
                                           "HASH_19", "FREQ_19",
                                           "HASH_20", "FREQ_20",
                                           "HASH_21", "FREQ_21",
                                           "HASH_22", "FREQ_22",
                                           "HASH_23", "FREQ_23",
                                           "HASH_24", "FREQ_24",
                                           "HASH_25", "FREQ_25",
                                           "HASH_26", "FREQ_26",
                                           "HASH_27", "FREQ_27",
                                           "HASH_28", "FREQ_28",
                                           "HASH_29", "FREQ_29",
                                           "HASH_30", "FREQ_30",
                                           };
    EXPECT_EQ(exp_sample, Agent::sample_names(dict));
    EXPECT_EQ(exp_policy, Agent::policy_names(dict));

    EXPECT_EQ(exp_sample, Agent::sample_names(agent_name));
    EXPECT_EQ(exp_policy, Agent::policy_names(agent_name));
}
