/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>

#include "geopm/json11.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_agent.h"
#include "geopm_hint.h"
#include "geopm_hash.h"

#include "Agent.hpp"
#include "FrequencyMapAgent.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_prof.h"
#include "geopm_test.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using geopm::FrequencyMapAgent;
using geopm::PlatformTopo;
using json11::Json;

class FrequencyMapAgentTest : public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            REGION_HASH_IDX,
            FREQ_CONTROL_IDX,
            UNCORE_MIN_CTL_IDX,
            UNCORE_MAX_CTL_IDX
        };
        enum policy_idx_e {
            DEFAULT = 0,  // M_POLICY_FREQ_DEFAULT
            UNCORE = 1,
            HASH_0 = 2,
            FREQ_0 = 3,
            HASH_1 = 4,
            FREQ_1 = 5,
        };

        void SetUp();
        void TearDown();
        static const int M_NUM_CPU = 1;
        static const size_t M_NUM_REGIONS = 5;
        std::unique_ptr<FrequencyMapAgent> m_agent;
        std::vector<std::string> m_region_names;
        std::vector<uint64_t> m_region_hash;
        std::vector<double> m_mapped_freqs;
        std::vector<double> m_default_policy;
        size_t m_num_policy;
        double m_freq_min;
        double m_freq_max;
        double m_freq_step;
        double m_freq_uncore_min;
        double m_freq_uncore_max;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void FrequencyMapAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
    .WillByDefault(Return(1));
    ON_CALL(*m_platform_io, push_signal("REGION_HASH", _, _))
    .WillByDefault(Return(REGION_HASH_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_FREQUENCY_CONTROL", _, _))
    .WillByDefault(Return(FREQ_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", _, _))
    .WillByDefault(Return(UNCORE_MIN_CTL_IDX));
    ON_CALL(*m_platform_io, push_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", _, _))
    .WillByDefault(Return(UNCORE_MAX_CTL_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
    .WillByDefault(Return(geopm::Agg::max));

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_freq_step = 100000000.0;
    m_freq_uncore_min = 1700000000;
    m_freq_uncore_max = 2100000000;
    ON_CALL(*m_platform_io, control_domain_type("CPU_FREQUENCY_CONTROL"))
    .WillByDefault(Return(GEOPM_DOMAIN_BOARD));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
    .WillByDefault(Return(m_freq_min));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
    .WillByDefault(Return(m_freq_max));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_STEP", GEOPM_DOMAIN_BOARD, 0))
    .WillByDefault(Return(m_freq_step));
    ON_CALL(*m_platform_io, read_signal("MSR::PERF_CTL:FREQ", _, _))
    .WillByDefault(Return(m_freq_max));
    ON_CALL(*m_platform_io, read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", GEOPM_DOMAIN_BOARD, 0))
    .WillByDefault(Return(m_freq_uncore_min));
    ON_CALL(*m_platform_io, read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", GEOPM_DOMAIN_BOARD, 0))
    .WillByDefault(Return(m_freq_uncore_max));

    m_region_names = {"mapped_region0", "mapped_region1", "mapped_region2", "mapped_region3", "mapped_region4"};
    m_region_hash = {0xeffa9a8d, 0x4abb08f3, 0xa095c880, 0x5d45afe, 0x71243e97};
    m_mapped_freqs = {m_freq_max, 2100000000.0, 2000000000.0, 1900000000.0, m_freq_min};
    ASSERT_LT(m_freq_min, 1.9e9);
    ASSERT_LT(2.1e9, m_freq_max);
    m_default_policy = {m_freq_max, NAN};

    for (size_t i = 0; i < M_NUM_REGIONS; ++i) {
        m_default_policy.push_back(static_cast<double>(m_region_hash[i]));
        m_default_policy.push_back(m_mapped_freqs[i]);
    }

    ASSERT_EQ(m_mapped_freqs.size(), m_region_names.size());
    ASSERT_EQ(m_mapped_freqs.size(), m_region_hash.size());

    std::map<uint64_t, double> frequency_map;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        frequency_map[m_region_hash[x]] = m_mapped_freqs[x];
    }

    m_agent = geopm::make_unique<FrequencyMapAgent>(*m_platform_io, *m_platform_topo);
    m_num_policy = m_agent->policy_names().size();

    EXPECT_CALL(*m_platform_io, control_domain_type("CPU_FREQUENCY_CONTROL"));
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_CALL(*m_platform_io, push_signal("REGION_HASH", _, _));
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_CONTROL", _, _));
    EXPECT_CALL(*m_platform_io, push_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", _, _));
    EXPECT_CALL(*m_platform_io, push_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", _, _));

    // leaf agent
    m_agent->init(0, {}, false);
}

void FrequencyMapAgentTest::TearDown()
{

}

TEST_F(FrequencyMapAgentTest, adjust_platform_map)
{
    {
        // expectations for initialization of controls
        EXPECT_CALL(*m_platform_io, read_signal("MSR::PERF_CTL:FREQ", _, _))
        .WillOnce(Return(m_freq_max));
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_freq_max));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MIN_CTL_IDX, m_freq_uncore_min));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MAX_CTL_IDX, m_freq_uncore_max));

        EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
        .WillOnce(Return(m_region_hash[0]));
        std::vector<double> tmp;
        m_agent->sample_platform(tmp);
        // initial all-NAN policy is accepted
        std::vector<double> empty_policy(m_num_policy, NAN);
        m_agent->adjust_platform(empty_policy);
        EXPECT_FALSE(m_agent->do_write_batch());
    }

    int num_samples = 3;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_mapped_freqs[x]))
        .Times(1);
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MIN_CTL_IDX, _)).Times(0);
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MAX_CTL_IDX, _)).Times(0);
        for (int sample = 0; sample < num_samples; ++sample) {
            EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
            .WillOnce(Return(m_region_hash[x]));
            std::vector<double> tmp;
            m_agent->sample_platform(tmp);
            m_agent->adjust_platform(m_default_policy);
            // only write when first entering the region
            if (sample == 0) {
                EXPECT_TRUE(m_agent->do_write_batch());
            }
        }
    }
    {
        // all-NAN policy after real policy is invalid
        std::vector<double> empty_policy(m_num_policy, NAN);
        GEOPM_EXPECT_THROW_MESSAGE(m_agent->adjust_platform(empty_policy),
                                   GEOPM_ERROR_INVALID,
                                   "invalid all-NAN policy");
    }
}

TEST_F(FrequencyMapAgentTest, adjust_platform_uncore)
{
    std::vector<double> policy(m_num_policy, NAN);
    {
        EXPECT_CALL(*m_platform_io, read_signal("MSR::PERF_CTL:FREQ", _, _));
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_freq_max));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MIN_CTL_IDX, m_freq_uncore_min));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MAX_CTL_IDX, m_freq_uncore_max));
        m_agent->adjust_platform(policy);
    }
    policy = m_default_policy;
    {
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_freq_max));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MIN_CTL_IDX, m_freq_uncore_min));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MAX_CTL_IDX, m_freq_uncore_min));
        policy[UNCORE] = m_freq_uncore_min;
        m_agent->adjust_platform(policy);
        EXPECT_TRUE(m_agent->do_write_batch());
        // don't write again if unchanged
        m_agent->adjust_platform(policy);
        EXPECT_FALSE(m_agent->do_write_batch());
    }
    {
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MIN_CTL_IDX, m_freq_uncore_max));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MAX_CTL_IDX, m_freq_uncore_max));
        policy[UNCORE] = m_freq_uncore_max;
        m_agent->adjust_platform(policy);
        EXPECT_TRUE(m_agent->do_write_batch());
        // don't write again if unchanged
        m_agent->adjust_platform(policy);
        EXPECT_FALSE(m_agent->do_write_batch());
    }
    // restore uncore to initial values if NAN
    {
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MIN_CTL_IDX, m_freq_uncore_min));
        EXPECT_CALL(*m_platform_io, adjust(UNCORE_MAX_CTL_IDX, m_freq_uncore_max));
        policy[UNCORE] = NAN;
        m_agent->adjust_platform(policy);
        EXPECT_TRUE(m_agent->do_write_batch());
        // don't write again if unchanged
        m_agent->adjust_platform(policy);
        EXPECT_FALSE(m_agent->do_write_batch());
    }
}

TEST_F(FrequencyMapAgentTest, split_policy)
{

    int num_children = 2;
    auto tree_agent = geopm::make_unique<FrequencyMapAgent>(*m_platform_io, *m_platform_topo);
    tree_agent->init(1, {num_children}, false);


    std::vector<double> policy(m_num_policy, NAN);
    std::vector<std::vector<double> > out_policy(num_children, policy);
    // do not send all NAN policy
    tree_agent->split_policy(policy, out_policy);
    EXPECT_FALSE(tree_agent->do_send_policy());

    // send first policy
    policy[DEFAULT] = m_freq_max;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_max, out_policy[0][DEFAULT]);
    EXPECT_EQ(m_freq_max, out_policy[1][DEFAULT]);

    // do not send if unchanged
    tree_agent->split_policy(policy, out_policy);
    EXPECT_FALSE(tree_agent->do_send_policy());

    // send if policy changed
    policy[0] = m_freq_min;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_min, out_policy[0][DEFAULT]);
    EXPECT_EQ(m_freq_min, out_policy[1][DEFAULT]);

    // send if uncore changed
    policy[1] = m_freq_max;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_min, out_policy[0][DEFAULT]);
    EXPECT_EQ(m_freq_min, out_policy[1][DEFAULT]);
    EXPECT_EQ(m_freq_max, out_policy[0][UNCORE]);
    EXPECT_EQ(m_freq_max, out_policy[1][UNCORE]);

    // NAN uncore is ignored
    policy[1] = NAN;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_FALSE(tree_agent->do_send_policy());

#ifdef GEOPM_DEBUG
    // NAN for a mapped region is invalid
    policy[2] = 0xabc;
    GEOPM_EXPECT_THROW_MESSAGE(tree_agent->split_policy(policy, out_policy),
                               GEOPM_ERROR_LOGIC,
                               "mapped region with no frequency assigned");
#endif
}

TEST_F(FrequencyMapAgentTest, name)
{
    EXPECT_EQ("frequency_map", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(FrequencyMapAgentTest, enforce_policy)
{
    const double core_limit = 1e9;
    const double uncore_limit = 2e9;
    std::vector<double> policy(m_num_policy, NAN);
    std::vector<double> empty_policy(m_num_policy, NAN);
    // policy with default core frequency only;
    {
        policy[DEFAULT] = core_limit;
        EXPECT_CALL(*m_platform_io,
                    write_control("CPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0, core_limit));
        EXPECT_CALL(*m_platform_io,
                    write_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO", _, _, _))
        .Times(0);
        EXPECT_CALL(*m_platform_io,
                    write_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO", _, _, _))
        .Times(0);
        m_agent->enforce_policy(policy);
    }

    // policy with default core and uncore frequencies
    {
        policy[DEFAULT] = core_limit;
        policy[UNCORE] = uncore_limit;
        EXPECT_CALL(*m_platform_io,
                    write_control("CPU_FREQUENCY_CONTROL", GEOPM_DOMAIN_BOARD, 0, core_limit));
        EXPECT_CALL(*m_platform_io,
                    write_control("MSR::UNCORE_RATIO_LIMIT:MIN_RATIO",
                                  GEOPM_DOMAIN_BOARD, 0, uncore_limit));
        EXPECT_CALL(*m_platform_io,
                    write_control("MSR::UNCORE_RATIO_LIMIT:MAX_RATIO",
                                  GEOPM_DOMAIN_BOARD, 0, uncore_limit));
        m_agent->enforce_policy(policy);
    }

    // all NAN policy is invalid
    {
        GEOPM_EXPECT_THROW_MESSAGE(m_agent->enforce_policy(empty_policy),
                                   GEOPM_ERROR_INVALID,
                                   "invalid all-NAN policy");
    }

    const std::vector<double> bad_policy(123, 100);
    EXPECT_THROW(m_agent->enforce_policy(bad_policy), geopm::Exception);
}

static Json get_freq_map_json_from_policy(const std::vector<double> &policy)
{
    std::array<char, 4096> policy_buf;
    EXPECT_EQ(0, geopm_agent_policy_json_partial("frequency_map", policy.size(),
                                                 policy.data(), policy_buf.size(),
                                                 policy_buf.data()));
    std::string parse_error;
    return Json::parse(policy_buf.data(), parse_error);
}

TEST_F(FrequencyMapAgentTest, policy_to_json)
{
    EXPECT_EQ(Json(Json::object{ { "FREQ_DEFAULT", 0 }, { "FREQ_UNCORE", 3e9 } }),
    get_freq_map_json_from_policy({ 0, 3e9 }));
    EXPECT_EQ(Json(Json::object{ { "FREQ_DEFAULT", 0 }, { "FREQ_UNCORE", 1e40 } }),
    get_freq_map_json_from_policy({ 0, 1e40 }));
    EXPECT_EQ(Json(Json::object{ { "FREQ_DEFAULT", 0 }, { "FREQ_UNCORE", 1e-40 } }),
    get_freq_map_json_from_policy({ 0, 1e-40 }));
}

TEST_F(FrequencyMapAgentTest, validate_policy)
{
    using ::testing::Pointwise;
    using ::testing::NanSensitiveDoubleEq;

    const std::vector<double> empty(m_num_policy, NAN);
    std::vector<double> policy;

    // valid policy is unmodified
    policy = empty;
    policy[DEFAULT] = m_freq_max;
    policy[UNCORE] = 1.0e9;
    policy[HASH_0] = 123;
    policy[FREQ_0] = m_freq_min;
    m_agent->validate_policy(policy);
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[DEFAULT]);
    EXPECT_EQ(1.0e9, policy[UNCORE]);
    EXPECT_EQ(123, policy[HASH_0]);
    EXPECT_EQ(m_freq_min, policy[FREQ_0]);
    EXPECT_TRUE(std::isnan(policy[HASH_1]));
    EXPECT_TRUE(std::isnan(policy[FREQ_1]));

    // gaps in mapped regions allowed
    policy = empty;
    policy[DEFAULT] = m_freq_max;
    policy[UNCORE] = 1.0e9;
    policy[HASH_1] = 123;
    policy[FREQ_1] = m_freq_min;
    m_agent->validate_policy(policy);
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[DEFAULT]);
    EXPECT_EQ(1.0e9, policy[UNCORE]);
    EXPECT_TRUE(std::isnan(policy[HASH_0]));
    EXPECT_TRUE(std::isnan(policy[FREQ_0]));
    EXPECT_EQ(123, policy[HASH_1]);
    EXPECT_EQ(m_freq_min, policy[FREQ_1]);

    // all-NAN policy is accepted
    policy = empty;
    m_agent->validate_policy(policy);
    EXPECT_TRUE(std::isnan(policy[DEFAULT]));

    // default must be set if not all NAN
    policy = empty;
    policy[UNCORE] = 1.0e9;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default frequency must be provided in policy");

    // default must be within system limits
    policy[DEFAULT] = m_freq_max + 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default frequency out of range");
    policy[DEFAULT] = m_freq_min - 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default frequency out of range");

    // cannot have same region with multiple freqs
    policy = empty;
    policy[DEFAULT] = m_freq_max;
    policy[HASH_0] = 123;
    policy[HASH_1] = 123;
    policy[FREQ_0] = m_freq_max;
    policy[FREQ_1] = m_freq_min;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "policy has multiple entries for region");

    // mapped region cannot have NAN frequency
    policy = empty;
    policy[DEFAULT] = m_freq_max;
    policy[HASH_0] = 123;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "mapped region with no frequency assigned");

    // cannot have frequency without region
    policy = empty;
    policy[DEFAULT] = m_freq_max;
    policy[FREQ_0] = m_freq_min;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "policy maps a NaN region with frequency");
}

TEST_F(FrequencyMapAgentTest, report_hash_freq_map)
{
    FrequencyMapAgent frequency_agent( {
        {0x000000003ddc81bf, 1000000000},
        {0x00000000644f9787, 2100000000},
        {0x00000000725e8066, 2100000000},
        {0x000000007b561f45, 2100000000},
        {0x00000000a74bbf35, 1200000000},
        {0x00000000d691da00, 1900000000},
        {0x8000000000000000, 2100000000}
    },
    {},
    *m_platform_io, *m_platform_topo
                                     );
    std::string reference_map =  "{0x000000003ddc81bf: 1000000000, "
                                 "0x00000000644f9787: 2100000000, "
                                 "0x00000000725e8066: 2100000000, "
                                 "0x000000007b561f45: 2100000000, "
                                 "0x00000000a74bbf35: 1200000000, "
                                 "0x00000000d691da00: 1900000000, "
                                 "0x8000000000000000: 2100000000}";

    auto result = frequency_agent.report_host();
    for (auto& key_value : result) {
        EXPECT_EQ(key_value.second, reference_map);
    }
}

TEST_F(FrequencyMapAgentTest, report_default_freq_hash)
{
    FrequencyMapAgent frequency_agent(
    {}, {
        {
            0x00000000a74bbf35,
            0x00000000d691da00,
            0x8000000000000000
        }
    },
    *m_platform_io, *m_platform_topo
    );
    std::string reference_map =  "{0x00000000a74bbf35: null, "
                                 "0x00000000d691da00: null, "
                                 "0x8000000000000000: null}";

    auto result = frequency_agent.report_host();
    for (auto& key_value : result) {
        EXPECT_EQ(key_value.second, reference_map);
    }
}

TEST_F(FrequencyMapAgentTest, report_both_map_and_set)
{
    FrequencyMapAgent frequency_agent( {
        {0x000000003ddc81bf, 1000000000},
        {0x00000000644f9787, 2100000000},
        {0x00000000725e8066, 2100000000},
        {0x000000007b561f45, 2100000000},
        {0x00000000a74bbf35, 1200000000},
        {0x00000000d691da00, 1900000000},
        {0x8000000000000000, 2100000000}
    }, {
        {
            0x00000000644f9789,
            0x000000007b561f47,
            0x00000000d691da02
        }
    },
    *m_platform_io, *m_platform_topo
                                     );
    std::string reference_map =  "{0x000000003ddc81bf: 1000000000, "
                                 "0x00000000644f9787: 2100000000, "
                                 "0x00000000644f9789: null, "
                                 "0x00000000725e8066: 2100000000, "
                                 "0x000000007b561f45: 2100000000, "
                                 "0x000000007b561f47: null, "
                                 "0x00000000a74bbf35: 1200000000, "
                                 "0x00000000d691da00: 1900000000, "
                                 "0x00000000d691da02: null, "
                                 "0x8000000000000000: 2100000000}";

    auto result = frequency_agent.report_host();
    for (auto& key_value : result) {
        EXPECT_EQ(key_value.second, reference_map);
    }
}

TEST_F(FrequencyMapAgentTest, report_neither_map_nor_set)
{
    FrequencyMapAgent frequency_agent(
        {},
        {},
        *m_platform_io, *m_platform_topo
    );
    std::string reference_map =  "{}";

    auto result = frequency_agent.report_host();
    for (auto& key_value : result) {
        EXPECT_EQ(key_value.second, reference_map);
    }
}
