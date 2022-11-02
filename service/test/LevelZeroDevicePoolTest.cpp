/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#include "MockLevelZero.hpp"
#include "LevelZeroDevicePoolImp.hpp"
#include "geopm_test.hpp"

using geopm::LevelZeroDevicePoolImp;
using geopm::Exception;
using testing::Return;

class LevelZeroDevicePoolTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();

        std::shared_ptr<MockLevelZero> m_levelzero;
};

void LevelZeroDevicePoolTest::SetUp()
{
    m_levelzero = std::make_shared<MockLevelZero>();
}

void LevelZeroDevicePoolTest::TearDown()
{
}

TEST_F(LevelZeroDevicePoolTest, device_count)
{
    const int num_gpu = 4;
    const int num_gpu_subdevice = 8;
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_EQ(num_gpu, m_device_pool.num_gpu(GEOPM_DOMAIN_GPU));
    EXPECT_EQ(num_gpu_subdevice, m_device_pool.num_gpu(GEOPM_DOMAIN_GPU_CHIP));
}

TEST_F(LevelZeroDevicePoolTest, subdevice_conversion_and_function)
{
    const int num_gpu = 4;
    const int num_gpu_subdevice = 8;
    const int num_subdevice_per_device = num_gpu_subdevice/num_gpu;

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    int value = 1500;
    std::vector<double> perf_value_chip_compute = {0.50, 0.51, 0.52, 0.53, 0.54, 0.55, 0.56, 0.57};
    std::vector<double> perf_value_chip_mem = {0.40, 0.41, 0.42, 0.43, 0.44, 0.45, 0.46, 0.47};
    int offset = 0;
    int domain_count = 1; //any non-zero number to ensure we don't throw
    for (int dev_idx = 0; dev_idx < num_gpu; ++dev_idx) {
        EXPECT_CALL(*m_levelzero, frequency_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        EXPECT_CALL(*m_levelzero, engine_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));

        EXPECT_CALL(*m_levelzero, performance_domain_count(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        EXPECT_CALL(*m_levelzero, performance_domain_count(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_MEMORY)).WillRepeatedly(Return(domain_count));

        for (int sub_idx = 0; sub_idx < num_subdevice_per_device; ++sub_idx) {
            EXPECT_CALL(*m_levelzero, frequency_status(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset));
            EXPECT_CALL(*m_levelzero, frequency_efficient(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_gpu_subdevice*10));
            EXPECT_CALL(*m_levelzero, frequency_min(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_gpu_subdevice*20));
            EXPECT_CALL(*m_levelzero, frequency_max(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_gpu_subdevice*30));

            EXPECT_CALL(*m_levelzero, active_time(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_gpu_subdevice*40));
            EXPECT_CALL(*m_levelzero, active_time_timestamp(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_gpu_subdevice*50));

            EXPECT_CALL(*m_levelzero, performance_factor(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(perf_value_chip_compute[offset]));
            EXPECT_CALL(*m_levelzero, performance_factor(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_MEMORY, sub_idx)).WillOnce(Return(perf_value_chip_mem[offset]));
            ++offset;
        }
    }
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    for (int sub_idx = 0; sub_idx < num_gpu_subdevice; ++sub_idx) {
        EXPECT_EQ(value+sub_idx, m_device_pool.frequency_status(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_gpu_subdevice*10, m_device_pool.frequency_efficient(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_gpu_subdevice*20, m_device_pool.frequency_min(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_gpu_subdevice*30, m_device_pool.frequency_max(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));

        EXPECT_EQ((uint64_t)(value+sub_idx+num_gpu_subdevice*40), m_device_pool.active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ((uint64_t)(value+sub_idx+num_gpu_subdevice*50), m_device_pool.active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));

        EXPECT_EQ((uint64_t)(value+sub_idx+num_gpu_subdevice*30), m_device_pool.active_time(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ((uint64_t)(value+sub_idx+num_gpu_subdevice*40), m_device_pool.active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));

        EXPECT_NO_THROW(m_device_pool.frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, value, value));

        EXPECT_NO_THROW(m_device_pool.frequency_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, value, value));

        EXPECT_EQ(perf_value_chip_compute[sub_idx], m_device_pool.performance_factor(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ(perf_value_chip_mem[sub_idx], m_device_pool.performance_factor(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY));

        //TODO: Add SUBDEVICE perf factor calls
        EXPECT_NO_THROW(m_device_pool.performance_factor_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, 0.5));
        EXPECT_NO_THROW(m_device_pool.performance_factor_control(GEOPM_DOMAIN_GPU_CHIP, sub_idx, MockLevelZero::M_DOMAIN_MEMORY, 0.5));
    }
}

TEST_F(LevelZeroDevicePoolTest, subdevice_conversion_error)
{
    const int num_gpu = 4;
    const int num_gpu_subdevice = 9;

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_GPU_CHIP, 0, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "GEOPM Requires the number of subdevices to be evenly divisible by the number of devices");
}

TEST_F(LevelZeroDevicePoolTest, domain_error)
{
    const int num_gpu = 4;
    const int num_gpu_subdevice = 8;

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    int dev_idx = 0;
    int domain_count = 0; //zero to cause a throw
    EXPECT_CALL(*m_levelzero, frequency_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
    EXPECT_CALL(*m_levelzero, engine_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    //Frequency
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_efficient(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_min(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_max(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");

    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the frequency domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_efficient(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the frequency domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_min(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the frequency domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_max(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the frequency domain");

    //Utilization
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_timestamp(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_pair(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");

    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the engine domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_timestamp(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the engine domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_pair(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" is not supported for the engine domain");

    //Energy & Power
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy_pair(GEOPM_DOMAIN_INVALID, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_INVALID)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.power_limit_tdp(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU_CHIP)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.power_limit_min(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU_CHIP)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.power_limit_max(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU_CHIP)+" is not supported for the power domain");
}

TEST_F(LevelZeroDevicePoolTest, subdevice_range_check)
{
    const int num_gpu = 4;
    const int num_gpu_subdevice = 8;

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_GPU_CHIP, num_gpu_subdevice, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "domain " +std::to_string(GEOPM_DOMAIN_GPU_CHIP)+ " idx "+std::to_string(num_gpu_subdevice)+" is out of range");
}

TEST_F(LevelZeroDevicePoolTest, device_range_check)
{
    const int num_gpu = 4;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy(GEOPM_DOMAIN_GPU, num_gpu, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_GPU)+" idx "+std::to_string(num_gpu)+" is out of range");
}

TEST_F(LevelZeroDevicePoolTest, device_function_check)
{
    const int num_gpu = 4;

    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU)).WillRepeatedly(Return(num_gpu));
    EXPECT_CALL(*m_levelzero, num_gpu(GEOPM_DOMAIN_GPU_CHIP)).WillRepeatedly(Return(num_gpu));

    int value = 1500;
    int offset = 0;
    for (int dev_idx = 0; dev_idx < num_gpu; ++dev_idx) {
        EXPECT_CALL(*m_levelzero, power_limit_tdp(dev_idx)).WillOnce(Return(value + offset));
        EXPECT_CALL(*m_levelzero, power_limit_min(dev_idx)).WillOnce(Return(value + offset + num_gpu * 10));
        EXPECT_CALL(*m_levelzero, power_limit_max(dev_idx)).WillOnce(Return(value + offset + num_gpu * 20));
        EXPECT_CALL(*m_levelzero, energy(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL, 0)).WillOnce(Return(value + offset + num_gpu * 30));
        EXPECT_CALL(*m_levelzero, energy_timestamp(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL, 0)).WillOnce(Return(value + offset + num_gpu * 35));
        EXPECT_CALL(*m_levelzero, power_domain_count(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(1));

        EXPECT_CALL(*m_levelzero, energy(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL, 0)).WillOnce(Return(value + offset + num_gpu * 40));
        EXPECT_CALL(*m_levelzero, energy_timestamp(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL, 0)).WillOnce(Return(value + offset + num_gpu * 45));
        EXPECT_CALL(*m_levelzero, power_domain_count(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL)).WillRepeatedly(Return(1));

        ++offset;
    }
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    for (int dev_idx = 0; dev_idx < num_gpu; ++dev_idx) {
        EXPECT_EQ(value + dev_idx, m_device_pool.power_limit_tdp(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ(value + dev_idx + num_gpu * 10, m_device_pool.power_limit_min(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ(value + dev_idx + num_gpu * 20, m_device_pool.power_limit_max(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ((uint64_t)(value + dev_idx + num_gpu * 30), m_device_pool.energy(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ((uint64_t)(value + dev_idx + num_gpu * 35), m_device_pool.energy_timestamp(GEOPM_DOMAIN_GPU, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ((uint64_t)(value + dev_idx + num_gpu * 40), m_device_pool.energy(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ((uint64_t)(value + dev_idx + num_gpu * 45), m_device_pool.energy_timestamp(GEOPM_DOMAIN_GPU_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL));
    }
}
