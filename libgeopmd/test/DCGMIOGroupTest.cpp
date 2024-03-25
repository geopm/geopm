/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <unistd.h>
#include <limits.h>

#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "config.h"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm/PluginFactory.hpp"
#include "DCGMIOGroup.hpp"
#include "geopm_test.hpp"
#include "MockDCGMDevicePool.hpp"
#include "MockPlatformTopo.hpp"

using geopm::DCGMIOGroup;
using geopm::PlatformTopo;
using geopm::Exception;
using testing::Return;

class DCGMIOGroupTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void write_affinitization(const std::string &affinitization_str);

        std::shared_ptr<MockDCGMDevicePool> m_device_pool;
        std::unique_ptr<MockPlatformTopo> m_platform_topo;
};

void DCGMIOGroupTest::SetUp()
{
    const int num_board = 1;
    const int num_package = 2;
    const int num_gpu = 4;
    const int num_core = 20;
    const int num_cpu = 40;

    m_device_pool = std::make_shared<MockDCGMDevicePool>();
    m_platform_topo = geopm::make_unique<MockPlatformTopo>();

    //Platform Topo prep
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_BOARD))
        .WillByDefault(Return(num_board));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_PACKAGE))
        .WillByDefault(Return(num_package));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_GPU))
        .WillByDefault(Return(num_gpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(num_cpu));
    ON_CALL(*m_platform_topo, num_domain(GEOPM_DOMAIN_CORE))
        .WillByDefault(Return(num_core));

    for (int cpu_idx = 0; cpu_idx < num_cpu; ++cpu_idx) {
        if (cpu_idx < 10) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(0));
        }
        else if (cpu_idx < 20) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(1));
        }
        else if (cpu_idx < 30) {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(2));
        }
        else {
            ON_CALL(*m_platform_topo, domain_idx(GEOPM_DOMAIN_GPU, cpu_idx))
                .WillByDefault(Return(3));
        }
    }

    EXPECT_CALL(*m_device_pool, num_device()).WillRepeatedly(Return(num_gpu));
}

void DCGMIOGroupTest::TearDown()
{
}

TEST_F(DCGMIOGroupTest, valid_signals)
{
    DCGMIOGroup dcgm_io(*m_platform_topo, *m_device_pool);
    for (const auto &sig : dcgm_io.signal_names()) {
        EXPECT_TRUE(dcgm_io.is_valid_signal(sig));
        EXPECT_NE(GEOPM_DOMAIN_INVALID, dcgm_io.signal_domain_type(sig));
        EXPECT_LT(-1, dcgm_io.signal_behavior(sig));
    }
}

TEST_F(DCGMIOGroupTest, push_control_adjust_write_batch)
{
    std::map<int, double> batch_value;
    DCGMIOGroup dcgm_io(*m_platform_topo, *m_device_pool);

    double mock_rate = 100;
    double mock_time = 6000;
    double mock_samples = 60000;

    batch_value[(dcgm_io.push_control("DCGM::FIELD_UPDATE_RATE",
                                    GEOPM_DOMAIN_BOARD, 0))] = mock_rate;
    EXPECT_CALL(*m_device_pool,
                update_rate(mock_rate * 1e6)).Times(1);

    batch_value[(dcgm_io.push_control("DCGM::MAX_STORAGE_TIME",
                                    GEOPM_DOMAIN_BOARD, 0))] = mock_time;
    EXPECT_CALL(*m_device_pool,
                max_storage_time(mock_time)).Times(1);

    batch_value[(dcgm_io.push_control("DCGM::MAX_SAMPLES",
                 GEOPM_DOMAIN_BOARD, 0))] = mock_samples;
    EXPECT_CALL(*m_device_pool,
                max_samples(mock_samples)).Times(1);

    for (auto& sv: batch_value) {
        // Given that we are mocking dcgmDevicePool the actual setting here doesn't matter
        EXPECT_NO_THROW(dcgm_io.adjust(sv.first, sv.second));
    }
    EXPECT_NO_THROW(dcgm_io.write_batch());
}

TEST_F(DCGMIOGroupTest, write_control)
{
    DCGMIOGroup dcgm_io(*m_platform_topo, *m_device_pool);

    double mock_rate = 100;
    double mock_time = 6000;
    double mock_samples = 60000;

    EXPECT_CALL(*m_device_pool,
                update_rate(mock_rate * 1e6)).Times(1);
    EXPECT_NO_THROW(dcgm_io.write_control("DCGM::FIELD_UPDATE_RATE",
                                          GEOPM_DOMAIN_BOARD, 0,
                                          mock_rate));

    EXPECT_CALL(*m_device_pool,
                max_storage_time(mock_time)).Times(1);
    EXPECT_NO_THROW(dcgm_io.write_control("DCGM::MAX_STORAGE_TIME",
                                          GEOPM_DOMAIN_BOARD, 0,
                                          mock_time));

    EXPECT_CALL(*m_device_pool,
                max_samples(mock_samples)).Times(1);
    EXPECT_NO_THROW(dcgm_io.write_control("DCGM::MAX_SAMPLES",
                                          GEOPM_DOMAIN_BOARD, 0,
                                          mock_samples));
}

TEST_F(DCGMIOGroupTest, read_signal_and_batch)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);

    std::vector<double> mock_sm_active = {1, 0.75, 0.5, 0.25};
    std::vector<double> mock_sm_occupancy = {0.8, 0.64, 0.35, 0.27};
    std::vector<double> mock_dram_active = {0, 0.78, 0.11, 0.33};
    std::vector<int> batch_idx;

    DCGMIOGroup dcgm_io(*m_platform_topo, *m_device_pool);

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, sample(gpu_idx, geopm::DCGMDevicePool::M_FIELD_ID_SM_ACTIVE)).WillRepeatedly(Return(mock_sm_active.at(gpu_idx)));
    }
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        batch_idx.push_back(dcgm_io.push_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, gpu_idx));
    }
    dcgm_io.read_batch();
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        double sm = dcgm_io.read_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, gpu_idx);
        double sm_batch = dcgm_io.sample(batch_idx.at(gpu_idx));

        EXPECT_DOUBLE_EQ(sm, mock_sm_active.at(gpu_idx));
        EXPECT_DOUBLE_EQ(sm, sm_batch);
    }

    mock_sm_active = {0.9, 0.45, 0.3, 0.29};
    //second round of testing with a modified value
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, sample(gpu_idx, geopm::DCGMDevicePool::M_FIELD_ID_SM_ACTIVE)).WillRepeatedly(Return(mock_sm_active.at(gpu_idx)));
    }
    dcgm_io.read_batch();
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        double sm = dcgm_io.read_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, gpu_idx);
        double sm_batch = dcgm_io.sample(batch_idx.at(gpu_idx));

        EXPECT_DOUBLE_EQ(sm, (mock_sm_active.at(gpu_idx)));
        EXPECT_DOUBLE_EQ(sm, sm_batch);
    }
}

TEST_F(DCGMIOGroupTest, read_signal)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);

    std::vector<double> mock_sm_active = {1, 0.75, 0.5, 0.25};
    std::vector<double> mock_sm_occupancy = {0.8, 0.64, 0.35, 0.27};
    std::vector<double> mock_dram_active = {0, 0.78, 0.11, 0.33};

    std::vector<int> active_process_list = {40961, 40962, 40963};

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, sample(gpu_idx, geopm::DCGMDevicePool::M_FIELD_ID_SM_ACTIVE)).WillRepeatedly(Return(mock_sm_active.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, sample(gpu_idx, geopm::DCGMDevicePool::M_FIELD_ID_SM_OCCUPANCY)).WillRepeatedly(Return(mock_sm_occupancy.at(gpu_idx)));
        EXPECT_CALL(*m_device_pool, sample(gpu_idx, geopm::DCGMDevicePool::M_FIELD_ID_DRAM_ACTIVE)).WillRepeatedly(Return(mock_dram_active.at(gpu_idx)));;
    }

    DCGMIOGroup dcgm_io(*m_platform_topo, *m_device_pool);

    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        double sm_active = dcgm_io.read_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, gpu_idx);
        double sm_active_alias = dcgm_io.read_signal("GPU_CORE_ACTIVITY", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(sm_active, sm_active_alias);
        EXPECT_DOUBLE_EQ(sm_active, mock_sm_active.at(gpu_idx));

        double sm_occupancy = dcgm_io.read_signal("DCGM::SM_OCCUPANCY", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(sm_occupancy, mock_sm_occupancy.at(gpu_idx));

        double dram_active = dcgm_io.read_signal("DCGM::DRAM_ACTIVE", GEOPM_DOMAIN_GPU, gpu_idx);
        double dram_active_alias = dcgm_io.read_signal("GPU_UNCORE_ACTIVITY", GEOPM_DOMAIN_GPU, gpu_idx);
        EXPECT_DOUBLE_EQ(dram_active, dram_active_alias);
        EXPECT_DOUBLE_EQ(dram_active, mock_dram_active.at(gpu_idx));
    }
}

//Test case: Error path testing including:
//              - Attempt to push a signal at an invalid domain level
//              - Attempt to push an invalid signal
//              - Attempt to sample without a read_batch prior
//              - Attempt to read a signal at an invalid domain level
//              - Attempt to push a control at an invalid domain level
//              - Attempt to adjust a non-existent batch index
//              - Attempt to write a control at an invalid domain level
TEST_F(DCGMIOGroupTest, error_path)
{
    const int num_gpu = m_platform_topo->num_domain(GEOPM_DOMAIN_GPU);

    std::vector<int> batch_idx;

    std::vector<double> mock_sm_active = {0.22, 0.79, 0.65, 0.37};
    for (int gpu_idx = 0; gpu_idx < num_gpu; ++gpu_idx) {
        EXPECT_CALL(*m_device_pool, sample(gpu_idx, geopm::DCGMDevicePool::M_FIELD_ID_SM_ACTIVE)).WillRepeatedly(Return(mock_sm_active.at(gpu_idx)));
    }

    EXPECT_CALL(*m_device_pool, num_device()).WillOnce(Return(num_gpu-1));
    GEOPM_EXPECT_THROW_MESSAGE(DCGMIOGroup dcgm_io_fail(*m_platform_topo, *m_device_pool), GEOPM_ERROR_INVALID, "DCGM enabled device count does not match GPU count");
    EXPECT_CALL(*m_device_pool, num_device()).WillRepeatedly(Return(num_gpu));

    DCGMIOGroup dcgm_io(*m_platform_topo, *m_device_pool);

    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.sample(0),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.read_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_BOARD, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_signal("DCGM::INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "signal_name DCGM::INVALID not valid for DCGMIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.read_signal("DCGM::INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "DCGM::INVALID not valid for DCGMIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_control("DCGM::FIELD_UPDATE_RATE", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "domain_type must be");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.adjust(0, 12345.6),
                               GEOPM_ERROR_INVALID, "batch_idx 0 out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.write_control("DCGM::FIELD_UPDATE_RATE", GEOPM_DOMAIN_GPU, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_type must be");

    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_control("DCGM::INVALID", GEOPM_DOMAIN_GPU, 0),
                               GEOPM_ERROR_INVALID, "control_name DCGM::INVALID not valid for DCGMIOGroup");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.write_control("DCGM::INVALID", GEOPM_DOMAIN_GPU, 0, 1530000000),
                               GEOPM_ERROR_INVALID, "DCGM::INVALID not valid for DCGMIOGroup");

    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, num_gpu),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.read_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, num_gpu),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.read_signal("DCGM::SM_ACTIVE", GEOPM_DOMAIN_GPU, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");

    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, num_gpu),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.push_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, -1),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.write_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, num_gpu, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
    GEOPM_EXPECT_THROW_MESSAGE(dcgm_io.write_control("DCGM::MAX_SAMPLES", GEOPM_DOMAIN_BOARD, -1, 1530000000),
                               GEOPM_ERROR_INVALID, "domain_idx out of range");
}
