/*
 * Copyright (c) 2015 - 2024, Intel Corporation
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
#include "FFNetAgent.hpp"
#include "DomainNetMap.hpp"
#include "RegionHintRecommender.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Agg.hpp"
#include "MockPlatformIO.hpp"
#include "MockPlatformTopo.hpp"
#include "MockDomainNetMap.hpp"
#include "MockRegionHintRecommender.hpp"
#include "MockWaiter.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_prof.h"
#include "geopm_test.hpp"

using ::testing::Mock;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Sequence;
using ::testing::Return;
using ::testing::AtLeast;
using geopm::FFNetAgent;
using geopm::PlatformTopo;
using geopm::DomainNetMap;
using geopm::RegionHintRecommender;
using testing::Throw;
using json11::Json;

class FFNetAgentTest : public ::testing::Test
{
    protected:
        enum mock_pio_idx_e {
            CPU_FREQ_MIN_CTL_IDX,
            CPU_FREQ_MAX_CTL_IDX,
            GPU_FREQ_MIN_CTL_IDX,
            GPU_FREQ_MAX_CTL_IDX
        };
        enum policy_idx_e {
            POLICY_PHI,
            NUM_POLICY
        };

        int init(bool m_do_gpu);
        void construct();
        int construct_and_init(bool m_do_gpu);
        static constexpr int M_NUM_PKG = 2;
        static constexpr int M_NUM_GPU = 6;

        std::vector<double> m_default_policy = {0.5};
        const std::map<std::string, double> M_REGION_CLASS = {{"dgemm", 0.75},
                                                              {"stream", 0.25}};
    
        std::unique_ptr<FFNetAgent> m_agent;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;

        std::map<std::pair<geopm_domain_e, int>, std::shared_ptr<MockDomainNetMap> > m_net_map;
        std::map<geopm_domain_e, std::shared_ptr<MockRegionHintRecommender> > m_freq_recommender;
};

int FFNetAgentTest::init(bool do_gpu)
{
    int num_gpu = do_gpu ? M_NUM_GPU : 0;

    // Set up mocks
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    for (int idx = 0; idx < M_NUM_PKG; ++idx) {
        m_net_map[std::make_pair(GEOPM_DOMAIN_PACKAGE, idx)]
            = std::make_shared<MockDomainNetMap>();
    }
    for (int idx = 0; idx < num_gpu; ++idx) {
        m_net_map[std::make_pair(GEOPM_DOMAIN_GPU, idx)]
            = std::make_shared<MockDomainNetMap>();
    }

    m_freq_recommender[GEOPM_DOMAIN_PACKAGE]
        = std::make_shared<MockRegionHintRecommender>();
    if (do_gpu) {
        m_freq_recommender[GEOPM_DOMAIN_GPU]
            = std::make_shared<MockRegionHintRecommender>();
    }

    // Platform Topo Calls
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(M_NUM_PKG));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(num_gpu));

    // Platform IO Calls
    ON_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Return(CPU_FREQ_MIN_CTL_IDX));
    ON_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .WillByDefault(Return(CPU_FREQ_MAX_CTL_IDX));

    if (do_gpu) {
        ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_GPU, _))
            .WillByDefault(Return(GPU_FREQ_MIN_CTL_IDX));
        ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU, _))
            .WillByDefault(Return(GPU_FREQ_MAX_CTL_IDX));
    }
    
    // Test init: ask for number of domains
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .Times(1);
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(do_gpu ? 2 : 1);

    // Test init: push controls 
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _))
        .Times(num_gpu);
    EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _))
        .Times(num_gpu);

    // Test init: Initializing MSRs that require it
    EXPECT_CALL(*m_platform_io, write_control("MSR::PQR_ASSOC:RMID", _, _, 0));
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:RMID", _, _, 0));
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:EVENT_ID", _, _, 2));

    return num_gpu;
}

void FFNetAgentTest::construct()
{
    std::map<std::pair<geopm_domain_e, int>, std::shared_ptr<DomainNetMap> > net_map_arg;
    std::map<geopm_domain_e, std::shared_ptr<RegionHintRecommender> > freq_recommender_arg;

    // This is needed because the map types cannot contain Mock to instantiate ffnetagent
    for (auto iter : m_net_map) {
        net_map_arg[iter.first] = iter.second;
    }
    for (auto iter : m_freq_recommender) {
        freq_recommender_arg[iter.first] = iter.second;
    }

    std::shared_ptr<geopm::Waiter> waiter = std::make_unique<MockWaiter>();
    m_agent = geopm::make_unique<FFNetAgent>(
            *m_platform_io,
            *m_platform_topo,
            net_map_arg,
            freq_recommender_arg,
            waiter);
    m_agent->init(0, {}, false); 
}

int FFNetAgentTest::construct_and_init(bool do_gpu)
{
    int num_gpu = init(do_gpu);
    construct();
    return num_gpu;
}

TEST_F(FFNetAgentTest, agent_name)
{
    construct_and_init(true);
    EXPECT_EQ("ffnet", m_agent->plugin_name());
    EXPECT_NE("bad_string", m_agent->plugin_name());
}

TEST_F(FFNetAgentTest, policy_names)
{
    std::vector<std::string> policy_names;

    construct_and_init(true);
    policy_names = m_agent->policy_names();
    EXPECT_EQ((std::size_t)1, policy_names.size());
    EXPECT_EQ("PERF_ENERGY_BIAS", policy_names.at(0));

}


// Test validate_policy: Accept all-nan policy
TEST_F(FFNetAgentTest, validate_empty_policy)
{
    construct_and_init(true);
    std::vector<double> empty_policy(NUM_POLICY, NAN);

    m_agent->validate_policy(empty_policy);
    EXPECT_EQ(0, empty_policy[POLICY_PHI]);
}

// Test validate_policy: Error if size != NUM_POLICY
TEST_F(FFNetAgentTest, validate_badsize_policy)
{
    construct_and_init(true);
    std::vector<double> policy(NUM_POLICY+3, 0);

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "policy vector not correctly sized.");
}

// Test validate_policy: Error if phi < 0 or phi > 1
TEST_F(FFNetAgentTest, validate_badphi_policy)
{
    construct_and_init(true);
    std::vector<double> policy(NUM_POLICY, NAN);

    policy[POLICY_PHI] = 1.5;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "PERF_ENERGY_BIAS is out of range (should be 0-1).");

    policy[POLICY_PHI] = -2.0;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "PERF_ENERGY_BIAS is out of range (should be 0-1).");
                               
}

// Test validate_policy: All good if phi [0,1]
TEST_F(FFNetAgentTest, validate_good_policy)
{
    construct_and_init(true);

    m_agent->validate_policy(m_default_policy);
    ASSERT_EQ(NUM_POLICY, m_default_policy.size());
}

// Test adjust_platform: NAN cpu and gpu freq recommendation = m_write_batch=false
TEST_F(FFNetAgentTest, adjust_platform_nans)
{
    int num_gpu = construct_and_init(true);
    int ncalls = 0;

    // Call to DomainNetMap to get regions
    for (const auto &net_map_pair : m_net_map) {
        ON_CALL(*net_map_pair.second, last_output())
            .WillByDefault(Return(M_REGION_CLASS));
        EXPECT_CALL(*net_map_pair.second, last_output());
    }
    // Call to RegionHintRecommender to get NAN recommended freq
    for (const auto &freq_rec_pair : m_freq_recommender) {
        ON_CALL(*freq_rec_pair.second,
                recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
            .WillByDefault(Return(NAN));
        //Get number of expected calls based on domain type
        if (freq_rec_pair.first == GEOPM_DOMAIN_PACKAGE) {
            ncalls = M_NUM_PKG;
        }
        else if (freq_rec_pair.first == GEOPM_DOMAIN_GPU) {
            ncalls = num_gpu;
        }

        EXPECT_CALL(*freq_rec_pair.second,
                    recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
            .Times(ncalls);
    }

    m_agent->adjust_platform(m_default_policy);
    EXPECT_FALSE(m_agent->do_write_batch());
}

// Test adjust_platform: New cpu freq recommendation means cpu freq is set
TEST_F(FFNetAgentTest, adjust_platform_all)
{
    int num_gpu = construct_and_init(true);
    int cpu_req = 1.2e9;
    int gpu_req = 1.0e9;
    int ncalls = 0;

    // Call to DomainNetMap to get regions
    for (const auto &net_map_pair : m_net_map) {
        ON_CALL(*net_map_pair.second, last_output())
            .WillByDefault(Return(M_REGION_CLASS));
        EXPECT_CALL(*net_map_pair.second, last_output())
            .Times(1);
    }
    // Call to RegionHintRecommender to get recommended freq
    for (const auto &freq_rec_pair : m_freq_recommender) {
        if (freq_rec_pair.first == GEOPM_DOMAIN_PACKAGE) {
            ncalls = M_NUM_PKG;
            ON_CALL(*freq_rec_pair.second,
                    recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
                .WillByDefault(Return(cpu_req));
        }
        else if (freq_rec_pair.first == GEOPM_DOMAIN_GPU) {
            ncalls = num_gpu;
            ON_CALL(*freq_rec_pair.second,
                    recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
                .WillByDefault(Return(gpu_req));
        }

        EXPECT_CALL(*freq_rec_pair.second,
                    recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
            .Times(ncalls);
    }

    EXPECT_CALL(*m_platform_io, adjust(CPU_FREQ_MIN_CTL_IDX, cpu_req))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, adjust(CPU_FREQ_MAX_CTL_IDX, cpu_req))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MIN_CTL_IDX, gpu_req))
        .Times(num_gpu);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MAX_CTL_IDX, gpu_req))
        .Times(num_gpu);

    m_agent->adjust_platform(m_default_policy);

    EXPECT_TRUE(m_agent->do_write_batch());

}

// Test adjust_platform: Do not get gpu freq recommendation when do_gpu=False
TEST_F(FFNetAgentTest, adjust_platform_no_gpu)
{
    int num_gpu = construct_and_init(false);
    int cpu_req = 1.2e9;
    int gpu_req = 1.0e9;

    // Call to DomainNetMap to get regions
    for (const auto &net_map_pair : m_net_map) {
        if (net_map_pair.first.first == GEOPM_DOMAIN_PACKAGE) {
            ON_CALL(*net_map_pair.second, last_output())
                .WillByDefault(Return(M_REGION_CLASS));
            EXPECT_CALL(*net_map_pair.second, last_output());
        }
    }
    // Call to RegionHintRecommender to get recommended freq
    for (const auto &freq_rec_pair : m_freq_recommender) {
        if (freq_rec_pair.first == GEOPM_DOMAIN_PACKAGE) {
            ON_CALL(*freq_rec_pair.second,
                    recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
                .WillByDefault(Return(cpu_req));
            EXPECT_CALL(*freq_rec_pair.second,
                        recommend_frequency(M_REGION_CLASS, m_default_policy[POLICY_PHI]))
                .Times(M_NUM_PKG);
        }
    }

    EXPECT_CALL(*m_platform_io, adjust(CPU_FREQ_MIN_CTL_IDX, cpu_req))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, adjust(CPU_FREQ_MAX_CTL_IDX, cpu_req))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MIN_CTL_IDX, gpu_req))
        .Times(num_gpu);
    EXPECT_CALL(*m_platform_io, adjust(GPU_FREQ_MAX_CTL_IDX, gpu_req))
        .Times(num_gpu);

    m_agent->adjust_platform(m_default_policy);

    EXPECT_TRUE(m_agent->do_write_batch());

}

// Test sample_platform: All signals are queried when do_gpu=True
TEST_F(FFNetAgentTest, sample_platform)
{
    construct_and_init(true);

    for (const auto &net_map_pair : m_net_map) {
        EXPECT_CALL(*net_map_pair.second, sample());
    }

    std::vector<double> tmp;
    m_agent->sample_platform(tmp);
}

// Test sample_platform: No GPU signals are queried when do_gpu=False
TEST_F(FFNetAgentTest, sample_platform_no_gpu)
{
    construct_and_init(false);

    for (const auto &net_map_pair : m_net_map) {
        if (net_map_pair.first.first == GEOPM_DOMAIN_PACKAGE) {
            EXPECT_CALL(*net_map_pair.second, sample())
                .Times(1);
        }
        else if (net_map_pair.first.first == GEOPM_DOMAIN_GPU) {
            EXPECT_CALL(*net_map_pair.second, sample())
                .Times(0);
        }
    }

    std::vector<double> tmp;
    m_agent->sample_platform(tmp);
}

// Test trace_names 
TEST_F(FFNetAgentTest, trace_names)
{
    construct_and_init(true);
    std::vector<std::string> retval;
    std::vector<std::string> cpu_region_names = {"aib", "stream"};
    std::vector<std::string> gpu_region_names = {"parres"};

    std::vector<std::string> expect_val = {"aib_cpu_0", "stream_cpu_0", "aib_cpu_1", "stream_cpu_1",
                                           "parres_gpu_0", "parres_gpu_1", "parres_gpu_2",
                                           "parres_gpu_3", "parres_gpu_4", "parres_gpu_5"};

    for (const auto &net_map_pair : m_net_map) {
        if (net_map_pair.first.first == GEOPM_DOMAIN_PACKAGE) {
            ON_CALL(*net_map_pair.second, trace_names())
                .WillByDefault(Return(cpu_region_names));
        }
        else if (net_map_pair.first.first == GEOPM_DOMAIN_GPU) {
            ON_CALL(*net_map_pair.second, trace_names())
                .WillByDefault(Return(gpu_region_names));
        }

        EXPECT_CALL(*net_map_pair.second, trace_names());
    }

    retval = m_agent->trace_names();

    EXPECT_EQ(expect_val.size(), retval.size());

    for (std::size_t idx = 0; idx < expect_val.size() ; idx++) {
        EXPECT_EQ(retval.at(idx), expect_val.at(idx));
    }
}

// Test trace_names no GPU
TEST_F(FFNetAgentTest, trace_names_no_gpu)
{
    construct_and_init(false);
    std::vector<std::string> retval;
    std::vector<std::string> cpu_region_names = {"aib", "stream"};
    std::vector<std::string> gpu_region_names = {"parres"};

    std::vector<std::string> expect_val = {"aib_cpu_0", "stream_cpu_0", "aib_cpu_1", "stream_cpu_1"};

    for (const auto &net_map_pair : m_net_map) {
        if (net_map_pair.first.first == GEOPM_DOMAIN_PACKAGE) {
            ON_CALL(*net_map_pair.second, trace_names())
                .WillByDefault(Return(cpu_region_names));
            EXPECT_CALL(*net_map_pair.second, trace_names());
        }
        else if (net_map_pair.first.first == GEOPM_DOMAIN_GPU) {
            ON_CALL(*net_map_pair.second, trace_names())
                .WillByDefault(Return(gpu_region_names));
            EXPECT_CALL(*net_map_pair.second, trace_names())
                .Times(0);
        }

    }

    retval = m_agent->trace_names();

    EXPECT_EQ(expect_val.size(), retval.size());

    for (std::size_t idx = 0; idx < expect_val.size() ; idx++) {
        EXPECT_EQ(retval.at(idx), expect_val.at(idx));
    }
}

// Test trace_values
TEST_F(FFNetAgentTest, trace_values)
{
    size_t num_gpu = construct_and_init(true);
    std::vector<std::vector<double>> cpu_probs;
    std::vector<std::vector<double>> gpu_probs;
    std::vector<double> expect_val;

    for (size_t idx = 0; idx < M_NUM_PKG; ++idx) {
        cpu_probs.push_back(std::vector<double>({1+(double)idx, 2}));
        expect_val.push_back(1+(double)idx);
        expect_val.push_back(2);

        
    }
    for (size_t idx = 0; idx < num_gpu; ++idx) {
        gpu_probs.push_back(std::vector<double>({(double)idx}));
        expect_val.push_back((double)idx);
    }

    for (const auto &net_map_pair : m_net_map) {
        if (net_map_pair.first.first == GEOPM_DOMAIN_PACKAGE) {
            ON_CALL(*net_map_pair.second, trace_values())
                .WillByDefault(Return(cpu_probs.at(net_map_pair.first.second)));
        }
        if (net_map_pair.first.first == GEOPM_DOMAIN_GPU) {
            ON_CALL(*net_map_pair.second, trace_values())
                .WillByDefault(Return(gpu_probs.at(net_map_pair.first.second)));
        }

        EXPECT_CALL(*net_map_pair.second, trace_values());
    }

    std::vector<double> retval(expect_val.size());
    m_agent->trace_values(retval);

    for (std::size_t idx = 0; idx < expect_val.size() ; idx++) {
        EXPECT_EQ(retval.at(idx), expect_val.at(idx));
    }
}

// Test trace_values no gpu
TEST_F(FFNetAgentTest, trace_values_no_gpu)
{
    construct_and_init(false);
    std::vector<std::vector<double>> cpu_probs;
    std::vector<double> expect_val;

    for (size_t idx = 0; idx < M_NUM_PKG; ++idx) {
        cpu_probs.push_back(std::vector<double>({1+(double)idx, 2}));
        expect_val.push_back(1+(double)idx);
        expect_val.push_back(2);
    }

    for (const auto &net_map_pair : m_net_map) {
        if (net_map_pair.first.first == GEOPM_DOMAIN_PACKAGE) {
            ON_CALL(*net_map_pair.second, trace_values())
                .WillByDefault(Return(cpu_probs.at(net_map_pair.first.second)));
            EXPECT_CALL(*net_map_pair.second, trace_values());
        }
        if (net_map_pair.first.first == GEOPM_DOMAIN_GPU) {
            ON_CALL(*net_map_pair.second, trace_values())
                .WillByDefault(Return(std::vector<double>({0})));
            EXPECT_CALL(*net_map_pair.second, trace_values())
                .Times(0);
        }
    }

    std::vector<double> retval(expect_val.size());
    m_agent->trace_values(retval);

    for (std::size_t idx = 0; idx < expect_val.size() ; idx++) {
        EXPECT_EQ(retval.at(idx), expect_val.at(idx));
    }
}
