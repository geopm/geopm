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

class FFNetAgentTest: public :: testing :: Test
{
    protected:
        enum mock_pio_idx_e {
            CPU_FREQ_CTL_IDX,
            GPU_FREQ_MIN_CTL_IDX,
            GPU_FREQ_MAX_CTL_IDX
        };
        enum policy_idx_e {
            PHI
        };

        void SetUp(); 
        void setup_gpu(bool m_do_gpu);
        void TearDown();
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
        //TODO: Add DomainNetMap and RegionHintRecommender mocks

};

void FFNetAgentTest::SetUp()
{
    m_platform_io = geopm::make_unique<MockPlatformIO>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();


    //Platform Topo Calls
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(M_NUM_PKG));
    
    //Platform IO Calls
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _))
        .Times(M_NUM_PKG)
        .WillRepeatedly(Return(m_cpu_freq_max));
    ON_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", _, _))
        .Times(M_NUM_PKG)
        .WillRepeatedly(Return(m_cpu_freq_min));

    //Test init: getting CPU freq range
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MIN_AVAIL", _, _));
    EXPECT_CALL(*m_platform_io, read_signal("CPU_FREQUENCY_MAX_AVAIL", _, _));
    //Test init: push controls for CPU
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MIN_CONTROL", _, _));
    EXPECT_CALL(*m_platform_io, push_control("CPU_FREQUENCY_MAX_CONTROL", _, _));
    //Test init: init controls for signals needed for nn
    EXPECT_CALL(*m_platform_io, write_control("MSR::PQR_ASSOC:RMID", _, _, 0));
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:RMID", _, _, 0));
    EXPECT_CALL(*m_platform_io, write_control("MSR::QM_EVTSEL:EVENT_ID", _, _, 0));
}

void FFNetAgentTest::setup_gpu(bool do_gpu)
{
    std::set<std::string> control_names = {
        "CPU_FREQUENCY_MAX_CONTROL"
    };

    if (do_gpu) {
        //Platform Topo Calls
        ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
            .WillByDefault(Return(M_NUM_GPU));
        //Platform IO Calls
        ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"))
            .WillByDefault(Return(GEOPM_DOMAIN_GPU));
        control_names.insert({
            "GPU_CORE_FREQUENCY_MAX_CONTROL",
            "GPU_CORE_FREQUENCY_MIN_CONTROL"});
        ON_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MAX_AVAIL", _, _))
            .Times(M_NUM_GPU)
            .WillRepeatedly(Return(m_gpu_freq_max));
        ON_CALL(*m_platform_io, read_signal("GPU_FREQUENCY_MIN_AVAIL", _, _))
            .Times(M_NUM_GPU)
            .WillRepeatedly(Return(m_gpu_freq_min));
    }
    //TODO: Fix
    else {
        //Platform Topo Calls
        ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
            .WillByDefault(Return(0));
        //Platform IO Calls
        ON_CALL(*m_platform_io, control_domain_type("GPU_CORE_FREQUENCY_MAX_CONTROL"))
            .WillByDefault(Return(GEOPM_DOMAIN_INVALID));
    }

    //GPU Push Controls
    ON_CALL(*m_platform_io, control_names())
        .WillByDefault(Return(control_names));
    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_GPU, 0))
        .WillByDefault(Return(GPU_FREQ_MIN_CONTROL_IDX));
    ON_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU, 0))
        .WillByDefault(Return(GPU_FREQ_MAX_CONTROL_IDX));

    //GPU Signals - frequency range
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", GEOPM_DOMAIN_GPU, 0))
        .WillByDefault(Return(m_freq_gpu_min));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", GEOPM_DOMAIN_GPU, 0))
        .WillByDefault(Return(m_freq_gpu_max));

    //GPU Signals = default frequency settings
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_CONTROL", GEOPM_DOMAIN_GPU, 0))
        .WillByDefault(Return(m_freq_gpu_min));
    ON_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_CONTROL", GEOPM_DOMAIN_GPU, 0))
        .WillByDefault(Return(m_freq_gpu_max));

    EXPECT_CALL(*m_platform_io, control_names());

    if (do_gpu) {
        //Test init: getting GPU freq range when do_gpu=True
        //Test init: GPU range is not queried when do_gpu=False [implicit]
        EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MIN_AVAIL", _, _))
            .Times(M_NUM_GPU);
        EXPECT_CALL(*m_platform_io, read_signal("GPU_CORE_FREQUENCY_MAX_AVAIL", _, _))
            .Times(M_NUM_GPU);
        //Test init: push controls for GPU when do_gpu=True
        //Test init: GPU controls are not pushed when do_gpu=False [implicit]
        EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MIN_CONTROL", _, _))
            .Times(M_NUM_GPU);
        EXPECT_CALL(*m_platform_io, push_control("GPU_CORE_FREQUENCY_MAX_CONTROL", _, _))
            .Times(M_NUM_GPU);
    
    }
}


//TODO Tests

//Test validate_policy: Error if size != 1
//Test validate_policy: Accept all-nan policy
//Test validate_policy: Error if phi < 0 or phi > 1
//Test validate_policy: All good if phi [0,1]

//These tests require mocks of DomainNetMap and RegionHintRecommender
//Test adjust_platform: New cpu freq recommendation means cpu freq is set
//Test adjust_platform: New gpu freq recommendation means gpu freq is set when do_gpu=True
//Test adjust_platform: NAN cpu and gpu freq recommendation = m_write_batch=false
//Test adjust_platform: Do not get gpu freq recommendation when do_gpu=False

//Test sample_platform: All CPU signals are queried
//Test sample_platform: All GPU signals are queried when do_gpu=True
//Test sample_platform: No GPU signals are queried when do_gpu=False

//TODO: Test trace_names
