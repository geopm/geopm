/*
 * Copyright (c) 2015 - 2023, Intel Corporation
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
using testing::Throw;
using json11::Json;

class FrequencyMapAgentTest : public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            REGION_HASH_IDX,
            FREQ_CONTROL_IDX,
            CPU_UNCORE_MIN_CTL_IDX,
            CPU_UNCORE_MAX_CTL_IDX,
            GPU_FREQ_MIN_CONTROL_IDX,
            GPU_FREQ_MAX_CONTROL_IDX,
        };
        enum policy_idx_e {
            CPU_DEFAULT = 0,  // M_POLICY_FREQ_CPU_DEFAULT
            CPU_UNCORE = 1,
            GPU_DEFAULT = 2, // M_POLICY_FREQ_GPU_DEFAULT
            HASH_0 = 3,
            FREQ_0 = 4,
            HASH_1 = 5,
            FREQ_1 = 6,
        };

        void SetUp();
        void setup_gpu(bool do_gpu);
        void TearDown();
        void set_expectations_adjust_platform_init(bool do_gpu);
        static const int M_NUM_CPU = 3;   
        bool m_do_gpu = true;
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
        double m_freq_cpu_uncore_min;
        double m_freq_cpu_uncore_max;
        double m_freq_gpu_min;
        double m_freq_gpu_max;
        double m_freq_gpu_step;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void FrequencyMapAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(M_NUM_CPU));
    ON_CALL(*m_platform_io, push_signal("REGION_HASH", _, _))
        .WillByDefault(Return(REGION_HASH_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MAX_CONTROL", _, _))
        .WillByDefault(Return(FREQ_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _))
        .WillByDefault(Return(CPU_UNCORE_MIN_CTL_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _))
        .WillByDefault(Return(CPU_UNCORE_MAX_CTL_IDX));
    ON_CALL(*m_platform_io, agg_function(_))
        .WillByDefault(Return(geopm::Agg::max));

    m_freq_min = 1800000000.0;
    m_freq_max = 2200000000.0;
    m_freq_step = 100000000.0;
    m_freq_cpu_uncore_min = 1700000000.0;
    m_freq_cpu_uncore_max = 2100000000.0;
    m_freq_gpu_min = 5.0e8; 
    m_freq_gpu_max = 1.5e9;
    m_freq_gpu_step = 500000000.0;

    ON_CALL(*m_platform_io, control_domain_type("CPU_FREQUENCY_MAX_CONTROL"))
        .WillByDefault(Return(GEOPM_DOMAIN_BOARD));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_min));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_max));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_STEP", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_step));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_CONTROL", _, _))
        .WillByDefault(Return(m_freq_max));
    ON_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_cpu_uncore_min));
    ON_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_cpu_uncore_max));

    m_region_names = {"mapped_region0", "mapped_region1", "mapped_region2", "mapped_region3", "mapped_region4"};
    m_region_hash = {0xeffa9a8d, 0x4abb08f3, 0xa095c880, 0x5d45afe, 0x71243e97};
    m_mapped_freqs = {m_freq_max, 2100000000.0, 2000000000.0, 1900000000.0, m_freq_min};

    ASSERT_LT(m_freq_min, m_freq_max - 1e8);

    m_default_policy = {m_freq_max, NAN, NAN};

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

    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD));
    EXPECT_CALL(*m_platform_io, control_domain_type("CPU_FREQUENCY_MAX_CONTROL"));
    EXPECT_CALL(*m_platform_io, push_signal("REGION_HASH", _, _)).Times(M_NUM_CPU);
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MAX_CONTROL", _, _)).Times(M_NUM_CPU);
    EXPECT_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _));
    EXPECT_CALL(*m_platform_io, push_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _));


}

//Sets up GPU expecations and performs init. Every test must call this.
void FrequencyMapAgentTest::setup_gpu(bool do_gpu)
{
    std::set<std::string> control_names = {
        "CPU_FREQUENCY_MAX_CONTROL",
        "CPU_UNCORE_FREQUENCY_MIN_CONTROL",
        "CPU_UNCORE_FREQUENCY_MAX_CONTROL"};

    if (do_gpu) {
        ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"))
            .WillByDefault(Return(GEOPM_DOMAIN_BOARD));
        control_names.insert({
            "GPU_CORE_FREQUENCY_MAX_CONTROL",
            "GPU_CORE_FREQUENCY_MIN_CONTROL"});
    }
    else {
        ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"))
            .WillByDefault(Throw(geopm::Exception("(): default GPU frequency specified on a system with no GPUs.", GEOPM_ERROR_INVALID, __FILE__, __LINE__)));
    }

    ON_CALL(*m_platform_io, control_names())
        .WillByDefault(Return(control_names));
    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(GPU_FREQ_MIN_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(GPU_FREQ_MAX_CONTROL_IDX));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_max));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_min));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_max));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_min));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0))
        .WillByDefault(Return(m_freq_gpu_max));

    EXPECT_CALL(*m_platform_io, control_names());

    if (do_gpu) {
        EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", _, _));
        EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", _, _));
        EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _));
        EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _));
    }

    // leaf agent
    m_agent->init(0, {}, false);
}

void FrequencyMapAgentTest::TearDown()
{

}

void FrequencyMapAgentTest::set_expectations_adjust_platform_init(bool do_gpu)
{
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_CONTROL", _, _))
        .Times(M_NUM_CPU)
        .WillRepeatedly(Return(m_freq_max));
    EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_freq_max))
        .Times(M_NUM_CPU);
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CTL_IDX, m_freq_cpu_uncore_min));
    EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CTL_IDX, m_freq_cpu_uncore_max));
    if (do_gpu) {
        EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MIN_CONTROL_IDX, m_freq_gpu_max));
        EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MAX_CONTROL_IDX, m_freq_gpu_max));
    }

    return;
}


TEST_F(FrequencyMapAgentTest, adjust_platform_map)
{
    setup_gpu(m_do_gpu);
    {
        std::vector<double> empty_policy(m_num_policy, NAN);
        set_expectations_adjust_platform_init(m_do_gpu);
        m_agent->adjust_platform(empty_policy);
        // initial all-NAN policy is accepted
        EXPECT_TRUE(m_agent->do_write_batch());

        // expectations for sample platform call
        EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
            .Times(M_NUM_CPU)
            .WillRepeatedly(Return(m_region_hash[0]));

        std::vector<double> tmp;
        m_agent->sample_platform(tmp);
    }
    int num_samples = 3;
    for (size_t x = 0; x < M_NUM_REGIONS; x++) {
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, m_mapped_freqs[x]))
            .Times(M_NUM_CPU);
        EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CTL_IDX, _))
            .Times(0);
        EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CTL_IDX, _))
            .Times(0);
        EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MIN_CONTROL_IDX, _))
            .Times(0);
        EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MAX_CONTROL_IDX, _))
            .Times(0);
        for (int sample = 0; sample < num_samples; ++sample) {
            EXPECT_CALL(*m_platform_io, sample(REGION_HASH_IDX))
                .Times(M_NUM_CPU)
                .WillRepeatedly(Return(m_region_hash[x]));
            std::vector<double> tmp;
            m_agent->sample_platform(tmp);
            m_agent->adjust_platform(m_default_policy);
            // initial all-NAN policy is accepted
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
    setup_gpu(m_do_gpu);

    {
        set_expectations_adjust_platform_init(m_do_gpu);
        m_agent->adjust_platform(policy);
        // initial all-NAN policy is accepted
        EXPECT_TRUE(m_agent->do_write_batch());
    }
    policy[CPU_DEFAULT] = 1.3e9;
    {
        // write cpu only if changed, do not write uncore
        EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, policy[CPU_DEFAULT]))
            .Times(M_NUM_CPU);
        EXPECT_TRUE(m_agent->do_write_batch());
    }
    {
        // write if uncore is changed
        EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CTL_IDX, m_freq_cpu_uncore_max));
        EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CTL_IDX, m_freq_cpu_uncore_max));
        policy[CPU_UNCORE] = m_freq_cpu_uncore_max;
        m_agent->adjust_platform(policy);
        EXPECT_TRUE(m_agent->do_write_batch());
        // don't write again if unchanged
        m_agent->adjust_platform(policy);
        EXPECT_FALSE(m_agent->do_write_batch());
    }
    // restore uncore to initial values if NAN
    {
        EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MIN_CTL_IDX, m_freq_cpu_uncore_min));
        EXPECT_CALL(*m_platform_io, adjust(CPU_UNCORE_MAX_CTL_IDX, m_freq_cpu_uncore_max));
        policy[CPU_UNCORE] = NAN;
        m_agent->adjust_platform(policy);
        EXPECT_TRUE(m_agent->do_write_batch());
        // don't write again if unchanged
        m_agent->adjust_platform(policy);
        EXPECT_FALSE(m_agent->do_write_batch());
    }
}

TEST_F(FrequencyMapAgentTest, adjust_platform_gpu)
{
    setup_gpu(m_do_gpu);
    std::vector<double> policy(m_num_policy, NAN);
    set_expectations_adjust_platform_init(m_do_gpu);
    m_agent->adjust_platform(policy);
    // initial all-NAN policy is accepted
    EXPECT_TRUE(m_agent->do_write_batch());

    policy[CPU_DEFAULT] = 1.3e9;
    policy[GPU_DEFAULT] = 1.1e9;
    EXPECT_CALL(*m_platform_io, adjust(FREQ_CONTROL_IDX, policy[CPU_DEFAULT]))
        .Times(M_NUM_CPU);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MIN_CONTROL_IDX, policy[GPU_DEFAULT]));
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MAX_CONTROL_IDX, policy[GPU_DEFAULT]));
    m_agent->adjust_platform(policy);
    EXPECT_TRUE(m_agent->do_write_batch());

    //Change GPU frequency
    policy[GPU_DEFAULT] = 1.4e9;
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MIN_CONTROL_IDX, policy[GPU_DEFAULT]));
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MAX_CONTROL_IDX, policy[GPU_DEFAULT]));
    m_agent->adjust_platform(policy);
    EXPECT_TRUE(m_agent->do_write_batch());

    //Don't write if the GPU frequency is unchanged
    policy[GPU_DEFAULT] = 1.4e9;
    m_agent->adjust_platform(policy);
    EXPECT_FALSE(m_agent->do_write_batch());

    //Don't write if GPU frequency changes to NAN
    policy[GPU_DEFAULT] = NAN;
    m_agent->adjust_platform(policy);
    EXPECT_FALSE(m_agent->do_write_batch());
}

TEST_F(FrequencyMapAgentTest, split_policy)
{
    int num_children = 2;

    setup_gpu(m_do_gpu);
    auto tree_agent = geopm::make_unique<FrequencyMapAgent>(*m_platform_io, *m_platform_topo);
    tree_agent->init(1, {num_children}, false);


    std::vector<double> policy(m_num_policy, NAN);
    std::vector<std::vector<double> > out_policy(num_children, policy);
    // do not send all NAN policy
    tree_agent->split_policy(policy, out_policy);
    EXPECT_FALSE(tree_agent->do_send_policy());

    // send first policy
    policy[CPU_DEFAULT] = m_freq_max;
    policy[CPU_UNCORE] = m_freq_cpu_uncore_max;
    policy[GPU_DEFAULT] = m_freq_gpu_max;

    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_max, out_policy[0][CPU_DEFAULT]);
    EXPECT_EQ(m_freq_max, out_policy[1][CPU_DEFAULT]);
    EXPECT_EQ(m_freq_cpu_uncore_max, out_policy[0][CPU_UNCORE]);
    EXPECT_EQ(m_freq_cpu_uncore_max, out_policy[1][CPU_UNCORE]);
    EXPECT_EQ(m_freq_gpu_max, out_policy[0][GPU_DEFAULT]);
    EXPECT_EQ(m_freq_gpu_max, out_policy[1][GPU_DEFAULT]);

    // do not send if unchanged
    tree_agent->split_policy(policy, out_policy);
    EXPECT_FALSE(tree_agent->do_send_policy());

    // send if policy changed
    policy[0] = m_freq_min;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_min, out_policy[0][CPU_DEFAULT]);
    EXPECT_EQ(m_freq_min, out_policy[1][CPU_DEFAULT]);
    EXPECT_EQ(m_freq_cpu_uncore_max, out_policy[0][CPU_UNCORE]);
    EXPECT_EQ(m_freq_cpu_uncore_max, out_policy[1][CPU_UNCORE]);
    EXPECT_EQ(m_freq_gpu_max, out_policy[0][GPU_DEFAULT]);
    EXPECT_EQ(m_freq_gpu_max, out_policy[1][GPU_DEFAULT]);

    // send if uncore changed
    double m_new_uncore_freq = 1200000000.0;
    policy[CPU_UNCORE] = m_new_uncore_freq;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_min, out_policy[0][CPU_DEFAULT]);
    EXPECT_EQ(m_freq_min, out_policy[1][CPU_DEFAULT]);
    EXPECT_EQ(m_new_uncore_freq, out_policy[0][CPU_UNCORE]);
    EXPECT_EQ(m_new_uncore_freq, out_policy[1][CPU_UNCORE]);
    EXPECT_EQ(m_freq_gpu_max, out_policy[0][GPU_DEFAULT]);
    EXPECT_EQ(m_freq_gpu_max, out_policy[1][GPU_DEFAULT]);

    // send if gpu changed
    double m_new_gpu_freq = m_freq_gpu_min;
    policy[GPU_DEFAULT] = m_new_gpu_freq;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_TRUE(tree_agent->do_send_policy());
    ASSERT_EQ((size_t)num_children, out_policy.size());
    EXPECT_EQ(m_freq_min, out_policy[0][CPU_DEFAULT]);
    EXPECT_EQ(m_freq_min, out_policy[1][CPU_DEFAULT]);
    EXPECT_EQ(m_new_uncore_freq, out_policy[0][CPU_UNCORE]);
    EXPECT_EQ(m_new_uncore_freq, out_policy[1][CPU_UNCORE]);
    EXPECT_EQ(m_freq_gpu_min, out_policy[0][GPU_DEFAULT]);
    EXPECT_EQ(m_freq_gpu_min, out_policy[1][GPU_DEFAULT]);

    // NAN uncore is ignored
    policy[CPU_UNCORE] = NAN;
    tree_agent->split_policy(policy, out_policy);
    EXPECT_FALSE(tree_agent->do_send_policy());

#ifdef GEOPM_DEBUG
    // NAN for a mapped region is invalid
    policy[HASH_0] = 0xabc;
    GEOPM_EXPECT_THROW_MESSAGE(tree_agent->split_policy(policy, out_policy),
                               GEOPM_ERROR_LOGIC,
                               "mapped region with no frequency assigned");
#endif
}

TEST_F(FrequencyMapAgentTest, name)
{
    setup_gpu(m_do_gpu);
    EXPECT_EQ("frequency_map", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(FrequencyMapAgentTest, enforce_policy_default_cpu_only)
{
    std::vector<double> policy(m_num_policy, NAN);
    const double core_limit = 1e9;
    setup_gpu(0);

    policy[CPU_DEFAULT] = core_limit;
    EXPECT_CALL(*m_platform_io,
            write_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0, core_limit));
    EXPECT_CALL(*m_platform_io,
            write_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL", _, _, _))
        .Times(0);
    EXPECT_CALL(*m_platform_io,
            write_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL", _, _, _))
        .Times(0);
    EXPECT_CALL(*m_platform_io,
            write_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _, _))
        .Times(0);
    EXPECT_CALL(*m_platform_io,
            write_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _, _))
        .Times(0);
    m_agent->enforce_policy(policy);
}

TEST_F(FrequencyMapAgentTest, enforce_policy_default_only)
{
    const double core_limit = 1e9;
    const double uncore_limit = 2e9;
    const double gpu_limit = 1.2e9;
    std::vector<double> policy(m_num_policy, NAN);
    setup_gpu(m_do_gpu);

    policy[CPU_DEFAULT] = core_limit;
    policy[CPU_UNCORE] = uncore_limit;
    policy[GPU_DEFAULT] = gpu_limit;

    EXPECT_CALL(*m_platform_io,
            write_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_BOARD, 0, core_limit));
    EXPECT_CALL(*m_platform_io,
            write_control("CPU_UNCORE_FREQUENCY_MIN_CONTROL",
                GEOPM_DOMAIN_BOARD, 0, uncore_limit));
    EXPECT_CALL(*m_platform_io,
            write_control("CPU_UNCORE_FREQUENCY_MAX_CONTROL",
                GEOPM_DOMAIN_BOARD, 0, uncore_limit));
    EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_BOARD, 0));
    EXPECT_CALL(*m_platform_io,
            write_control("GPU_CORE_FREQUENCY_MIN_CONTROL",
                GEOPM_DOMAIN_BOARD, 0, gpu_limit));
    EXPECT_CALL(*m_platform_io,
            write_control("GPU_CORE_FREQUENCY_MAX_CONTROL",
                GEOPM_DOMAIN_BOARD, 0, gpu_limit));
    m_agent->enforce_policy(policy);
}

TEST_F(FrequencyMapAgentTest, enforce_policy_allnan_invalid)
{
    std::vector<double> empty_policy(m_num_policy, NAN);
    setup_gpu(m_do_gpu);

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->enforce_policy(empty_policy),
            GEOPM_ERROR_INVALID,
            "invalid all-NAN policy");
}

TEST_F(FrequencyMapAgentTest, enforce_policy_bad_size)
{
    const std::vector<double> bad_policy(123, 100);
    setup_gpu(m_do_gpu);
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

    setup_gpu(m_do_gpu);
    EXPECT_EQ(Json(Json::object{ { "FREQ_CPU_DEFAULT", 0 }, { "FREQ_CPU_UNCORE", 3e9 }, {"FREQ_GPU_DEFAULT", 0 } }),
              get_freq_map_json_from_policy({ 0, 3e9, 0}));
    EXPECT_EQ(Json(Json::object{ { "FREQ_CPU_DEFAULT", 0 }, { "FREQ_CPU_UNCORE", 1e40 }, {"FREQ_GPU_DEFAULT", 0 } }),
              get_freq_map_json_from_policy({ 0, 1e40, 0 }));
    EXPECT_EQ(Json(Json::object{ { "FREQ_CPU_DEFAULT", 0 }, { "FREQ_CPU_UNCORE", 1e-40 }, {"FREQ_GPU_DEFAULT", 0 } }),
              get_freq_map_json_from_policy({ 0, 1e-40, 0 }));
    EXPECT_EQ(Json(Json::object{ { "FREQ_CPU_DEFAULT", 1e9 }, { "FREQ_CPU_UNCORE", 1.2e9 }, {"FREQ_GPU_DEFAULT", 5e8 } }),
              get_freq_map_json_from_policy({ 1e9, 1.2e9, 5e8}));
}

TEST_F(FrequencyMapAgentTest, validate_policy)
{
    setup_gpu(m_do_gpu);

    const std::vector<double> empty(m_num_policy, NAN);
    std::vector<double> policy;

    // valid policy is unmodified
    policy = empty;
    policy[CPU_DEFAULT] = m_freq_max;
    policy[CPU_UNCORE] = 1.0e9;
    policy[GPU_DEFAULT] = 8.0e8;
    policy[HASH_0] = 123;
    policy[FREQ_0] = m_freq_min;
    m_agent->validate_policy(policy);
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[CPU_DEFAULT]);
    EXPECT_EQ(1.0e9, policy[CPU_UNCORE]);
    EXPECT_EQ(8.0e8, policy[GPU_DEFAULT]);
    EXPECT_EQ(123, policy[HASH_0]);
    EXPECT_EQ(m_freq_min, policy[FREQ_0]);
    EXPECT_TRUE(std::isnan(policy[HASH_1]));
    EXPECT_TRUE(std::isnan(policy[FREQ_1]));

    // gaps in mapped regions allowed
    policy = empty;
    policy[CPU_DEFAULT] = m_freq_max;
    policy[CPU_UNCORE] = 1.0e9;
    policy[GPU_DEFAULT] = 8.0e8;
    policy[HASH_1] = 123;
    policy[FREQ_1] = m_freq_min;
    m_agent->validate_policy(policy);
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[CPU_DEFAULT]);
    EXPECT_EQ(1.0e9, policy[CPU_UNCORE]);
    EXPECT_EQ(8.0e8, policy[GPU_DEFAULT]);
    EXPECT_TRUE(std::isnan(policy[HASH_0]));
    EXPECT_TRUE(std::isnan(policy[FREQ_0]));
    EXPECT_EQ(123, policy[HASH_1]);
    EXPECT_EQ(m_freq_min, policy[FREQ_1]);

    // all-NAN policy is accepted
    policy = empty;
    m_agent->validate_policy(policy);
    EXPECT_TRUE(std::isnan(policy[CPU_DEFAULT]));

    // default must be set if not all NAN
    policy = empty;
    policy[CPU_UNCORE] = 1.0e9;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default CPU frequency must be provided in policy");

    // default must be set if not all NAN
    policy = empty;
    policy[GPU_DEFAULT] = 1.0e9;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default CPU frequency must be provided in policy");

    // default must be within system limits
    policy[CPU_DEFAULT] = m_freq_max + 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default CPU frequency out of range");
    policy[CPU_DEFAULT] = m_freq_min - 1;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "default CPU frequency out of range");

    // cannot have same region with multiple freqs
    policy = empty;
    policy[CPU_DEFAULT] = m_freq_max;
    policy[HASH_0] = 123;
    policy[HASH_1] = 123;
    policy[FREQ_0] = m_freq_max;
    policy[FREQ_1] = m_freq_min;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "policy has multiple entries for region");

    // mapped region cannot have NAN frequency
    policy = empty;
    policy[CPU_DEFAULT] = m_freq_max;
    policy[HASH_0] = 123;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "mapped region with no frequency assigned");

    // cannot have frequency without region
    policy = empty;
    policy[CPU_DEFAULT] = m_freq_max;
    policy[FREQ_0] = m_freq_min;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "policy maps a NaN region with frequency");
}

TEST_F(FrequencyMapAgentTest, validate_policy_nogpu)
{
    const std::vector<double> empty(m_num_policy, NAN);
    std::vector<double> policy;
    setup_gpu(false);

    // valid policy is unmodified
    policy = empty;
    policy[CPU_DEFAULT] = m_freq_max;
    policy[CPU_UNCORE] = 1.0e9;
    policy[HASH_0] = 123;
    policy[FREQ_0] = m_freq_min;

    m_agent->validate_policy(policy);
    ASSERT_EQ(m_num_policy, policy.size());
    EXPECT_EQ(m_freq_max, policy[CPU_DEFAULT]);
    EXPECT_EQ(1.0e9, policy[CPU_UNCORE]);
    EXPECT_EQ(123, policy[HASH_0]);
    EXPECT_EQ(m_freq_min, policy[FREQ_0]);
    EXPECT_TRUE(std::isnan(policy[HASH_1]));
    EXPECT_TRUE(std::isnan(policy[FREQ_1]));

    //policy containing GPU freq throws an error
    policy[GPU_DEFAULT] = 8.0e8;

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy),
                               GEOPM_ERROR_INVALID,
                               "default GPU frequency specified on a system with no GPUs.");
}

TEST_F(FrequencyMapAgentTest, report_hash_freq_map)
{
    setup_gpu(m_do_gpu);
    FrequencyMapAgent frequency_agent(
        {
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
    setup_gpu(m_do_gpu);
    FrequencyMapAgent frequency_agent(
        {},
        {
            {0x00000000a74bbf35,
             0x00000000d691da00,
             0x8000000000000000}
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
    setup_gpu(m_do_gpu);
    FrequencyMapAgent frequency_agent(
        {
            {0x000000003ddc81bf, 1000000000},
            {0x00000000644f9787, 2100000000},
            {0x00000000725e8066, 2100000000},
            {0x000000007b561f45, 2100000000},
            {0x00000000a74bbf35, 1200000000},
            {0x00000000d691da00, 1900000000},
            {0x8000000000000000, 2100000000}
        },
        {
            {0x00000000644f9789,
             0x000000007b561f47,
             0x00000000d691da02}
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
    setup_gpu(m_do_gpu);
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
