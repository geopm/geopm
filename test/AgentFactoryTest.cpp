/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "geopm/PluginFactory.hpp"
#include "Agent.hpp"
#include "MonitorAgent.hpp"
#include "PowerBalancerAgent.hpp"
#include "PowerGovernorAgent.hpp"
#include "FixedFrequencyAgent.hpp"
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
    std::vector<std::string> exp_policy = {"CPU_POWER_LIMIT",
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
    std::vector<std::string> exp_policy = {"CPU_POWER_LIMIT"};
    EXPECT_EQ(exp_sample, Agent::sample_names(dict));
    EXPECT_EQ(exp_policy, Agent::policy_names(dict));

    EXPECT_EQ(exp_sample, Agent::sample_names(agent_name));
    EXPECT_EQ(exp_policy, Agent::policy_names(agent_name));
}

TEST(AgentFactoryTest, DISABLED_static_info_fixed_frequency)
{
    auto &factory = geopm::agent_factory();
    std::string agent_name = geopm::FixedFrequencyAgent::plugin_name();
    auto &dict = factory.dictionary(agent_name);
    EXPECT_EQ(4, Agent::num_policy(dict));
    EXPECT_EQ(0, Agent::num_sample(dict));
    EXPECT_EQ(4, Agent::num_policy(agent_name));
    EXPECT_EQ(0, Agent::num_sample(agent_name));
    std::vector<std::string> exp_sample = {};
    std::vector<std::string> exp_policy = {"ACCELERATOR_FREQUENCY",
                                           "CORE_FREQUENCY",
                                           "UNCORE_MIN_FREQUENCY",
                                           "UNCORE_MAX_FREQUENCY"};
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
    std::vector<std::string> exp_policy = {"FREQ_DEFAULT",
                                           "FREQ_UNCORE",
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
