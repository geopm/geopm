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
using geopm::MockDomainNetMap;
using geopm::MockRegionHintRecommender;
using testing::Throw;
using json11::Json;

class FFNetAgentTest: public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            CPU_FREQ_CTL_IDX,
            GPU_FREQ_MIN_CTL_IDX,
            GPU_FREQ_MAX_CTL_IDX
        };
        enum policy_idx_e {
            POLICY_PHI,
            NUM_POLICY
        };

        void construct_and_init(bool m_do_gpu);
        static const int M_NUM_PKG = 2;
        static const int M_NUM_GPU = 6;
        bool m_do_gpu = true;
        double m_cpu_freq_min = 1800000000.0;
        double m_cpu_freq_max = 2200000000.0;
        double m_gpu_freq_min = 800000000.0;
        double m_gpu_freq_max = 1600000000.0;
    
        std::unique_ptr<FFNetAgent> m_agent;
        std::unique_ptr<MockPlatformIO> m_platform_io;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;

        std::map<std::pair<geopm_domain_e, int>, std::unique_ptr<DomainNetMap> > m_net_map;
        std::map<geopm_domain_e, std::unique_ptr<RegionHintRecommender> > m_freq_recommender;
};

void FFNetAgentTest::construct_and_init(bool do_gpu)
{
    int num_gpu = do_gpu ? M_NUM_GPU : 0;

    //Set up mocks
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    for (int idx = 0; idx < M_NUM_PKG; ++idx) {
        m_net_map[std::make_pair(GEOPM_DOMAIN_PACKAGE, idx)]
            = geopm::make_unique<MockDomainNetMap>();
    }
    for (int idx = 0; idx < num_gpu; ++idx) {
        m_net_map[std::make_pair(GEOPM_DOMAIN_GPU, idx)]
            = geopm::make_unique<MockDomainNetMap>();
    }

    m_freq_recommender[GEOPM_DOMAIN_PACKAGE]
        = geopm::make_unique<MockRegionHintRecommender>();
    if (do_gpu) {
        m_freq_recommender[GEOPM_DOMAIN_GPU]
            = geopm::make_unique<MockRegionHintRecommender>();
    }

    //Platform Topo Calls
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(M_NUM_PKG));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(num_gpu));
   

    //Test init: ask for number of domains
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .Times(1);
    EXPECT_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .Times(2);

    //Test init: push controls 
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_PACKAGE, _))
        .Times(M_NUM_PKG);
    EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _))
        .Times(num_gpu);
    EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _))
        .Times(num_gpu);

    //Test init: Initializing MSRs that require it
    EXPECT_CALL(*m_platform_io, write_control("MSR::PQR_ASSOC:RMID", _, _, 0));
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:RMID", _, _, 0));
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:EVENT_ID", _, _, 2));

    m_agent = geopm::make_unique<FFNetAgent>(
            *m_platform_io,
            *m_platform_topo,
            m_net_map,
            m_freq_recommender
            );
    m_agent->init(0, {}, false); 
}

//Test validate_policy: Accept all-nan policy
TEST_F(FFNetAgentTest, test_validate_empty_policy)
{
    construct_and_init(true);
    std::vector<double> empty_policy(NUM_POLICY, NAN);

    m_agent->validate_policy(empty_policy);
    EXPECT_EQ(0, empty_policy[POLICY_PHI]);
}

//Test validate_policy: Error if size != NUM_POLICY
TEST_F(FFNetAgentTest, test_validate_badsize_policy)
{
    construct_and_init(m_do_gpu);
    std::vector<double> policy(NUM_POLICY+3, 0);

    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "policy vector not correctly sized.");
}

//Test validate_policy: Error if phi < 0 or phi > 1
TEST_F(FFNetAgentTest, test_validate_badphi_policy)
{
    construct_and_init(m_do_gpu);
    std::vector<double> policy(NUM_POLICY, NAN);

    policy[POLICY_PHI] = 1.5;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "CPU_PHI is out of range (should be 0-1).");

    policy[POLICY_PHI] = -2.0;
    GEOPM_EXPECT_THROW_MESSAGE(m_agent->validate_policy(policy), GEOPM_ERROR_INVALID,
                               "CPU_PHI is out of range (should be 0-1).");
                               
}
//Test validate_policy: All good if phi [0,1]
TEST_F(FFNetAgentTest, test_validate_good_policy)
{
    construct_and_init(m_do_gpu);
    std::vector<double> policy(NUM_POLICY, NAN);

    policy[POLICY_PHI] = 0.25;
    m_agent->validate_policy(policy);
    ASSERT_EQ(NUM_POLICY, policy.size());
    EXPECT_EQ(0.25, policy[POLICY_PHI]);

}


//TODO Tests


//Test adjust_platform: NAN cpu and gpu freq recommendation = m_write_batch=false
//Test adjust_platform: New cpu freq recommendation means cpu freq is set
//Test adjust_platform: New gpu freq recommendation means gpu freq is set when do_gpu=True
//Test adjust_platform: Do not get gpu freq recommendation when do_gpu=False

//Test sample_platform: All CPU signals are queried
//Test sample_platform: All GPU signals are queried when do_gpu=True
//Test sample_platform: No GPU signals are queried when do_gpu=False

//TODO: Test trace_names
