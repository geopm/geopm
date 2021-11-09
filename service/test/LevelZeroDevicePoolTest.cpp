/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;
    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)).WillRepeatedly(Return(num_accelerator_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    EXPECT_EQ(num_accelerator, m_device_pool.num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR));
    EXPECT_EQ(num_accelerator_subdevice, m_device_pool.num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP));
}

TEST_F(LevelZeroDevicePoolTest, subdevice_conversion_and_function)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;
    const int num_subdevice_per_device = num_accelerator_subdevice/num_accelerator;

    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)).WillRepeatedly(Return(num_accelerator_subdevice));

    int value = 1500;
    int offset = 0;
    int domain_count = 1; //any non-zero number to ensure we don't throw
    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_CALL(*m_levelzero, frequency_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        EXPECT_CALL(*m_levelzero, engine_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        for (int sub_idx = 0; sub_idx < num_subdevice_per_device; ++sub_idx) {
            EXPECT_CALL(*m_levelzero, frequency_status(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset));
            EXPECT_CALL(*m_levelzero, frequency_min(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*10));
            EXPECT_CALL(*m_levelzero, frequency_max(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*20));

            EXPECT_CALL(*m_levelzero, active_time(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*30));
            EXPECT_CALL(*m_levelzero, active_time_timestamp(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*40));
            ++offset;
        }
    }
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        EXPECT_EQ(value+sub_idx, m_device_pool.frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_accelerator_subdevice*10, m_device_pool.frequency_min(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_accelerator_subdevice*20, m_device_pool.frequency_max(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));

        EXPECT_EQ((uint64_t)(value+sub_idx+num_accelerator_subdevice*30), m_device_pool.active_time(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));
        EXPECT_EQ((uint64_t)(value+sub_idx+num_accelerator_subdevice*40), m_device_pool.active_time_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE));

        EXPECT_NO_THROW(m_device_pool.frequency_control(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, sub_idx, MockLevelZero::M_DOMAIN_COMPUTE, value, value));
    }
}

TEST_F(LevelZeroDevicePoolTest, subdevice_conversion_error)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 9;

    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)).WillRepeatedly(Return(num_accelerator_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, 0, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "GEOPM Requires the number of subdevices to be evenly divisible by the number of devices");
}

TEST_F(LevelZeroDevicePoolTest, domain_error)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;

    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)).WillRepeatedly(Return(num_accelerator_subdevice));

    int dev_idx = 0;
    int domain_count = 0; //zero to cause a throw
    EXPECT_CALL(*m_levelzero, frequency_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
    EXPECT_CALL(*m_levelzero, engine_domain_count(dev_idx, MockLevelZero::M_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    //Frequency
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_min(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_max(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");

    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" is not supported for the frequency domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_min(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" is not supported for the frequency domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_max(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" is not supported for the frequency domain");

    //Utilization
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_pair(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");

    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" is not supported for the engine domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" is not supported for the engine domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.active_time_pair(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" is not supported for the engine domain");

    //Energy & Power
    //GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+" is not supported for the power domain");
    //GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy_pair(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.power_limit_tdp(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.power_limit_min(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+" is not supported for the power domain");
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.power_limit_max(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, dev_idx, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+" is not supported for the power domain");
}

TEST_F(LevelZeroDevicePoolTest, subdevice_range_check)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;

    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)).WillRepeatedly(Return(num_accelerator_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP, num_accelerator_subdevice, MockLevelZero::M_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "domain " +std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR_CHIP)+ " idx "+std::to_string(num_accelerator_subdevice)+" is out of range");
}

TEST_F(LevelZeroDevicePoolTest, device_range_check)
{
    const int num_accelerator = 4;
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy(GEOPM_DOMAIN_BOARD_ACCELERATOR, num_accelerator, MockLevelZero::M_DOMAIN_ALL), GEOPM_ERROR_INVALID, "domain "+std::to_string(GEOPM_DOMAIN_BOARD_ACCELERATOR)+" idx "+std::to_string(num_accelerator)+" is out of range");
}

TEST_F(LevelZeroDevicePoolTest, device_function_check)
{
    const int num_accelerator = 4;

    EXPECT_CALL(*m_levelzero, num_accelerator(GEOPM_DOMAIN_BOARD_ACCELERATOR)).WillRepeatedly(Return(num_accelerator));

    int value = 1500;
    int offset = 0;
    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_CALL(*m_levelzero, power_limit_tdp(dev_idx)).WillOnce(Return(value+offset));
        EXPECT_CALL(*m_levelzero, power_limit_min(dev_idx)).WillOnce(Return(value+offset+num_accelerator*10));
        EXPECT_CALL(*m_levelzero, power_limit_max(dev_idx)).WillOnce(Return(value+offset+num_accelerator*20));
        EXPECT_CALL(*m_levelzero, energy(dev_idx)).WillOnce(Return(value+offset+num_accelerator*30));
        EXPECT_CALL(*m_levelzero, energy_timestamp(dev_idx)).WillOnce(Return(value+offset+num_accelerator*40));
        ++offset;
    }
    LevelZeroDevicePoolImp m_device_pool(*m_levelzero);

    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_EQ(value+dev_idx, m_device_pool.power_limit_tdp(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ(value+dev_idx+num_accelerator*10, m_device_pool.power_limit_min(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ(value+dev_idx+num_accelerator*20, m_device_pool.power_limit_max(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ((uint64_t)(value+dev_idx+num_accelerator*30), m_device_pool.energy(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL));
        EXPECT_EQ((uint64_t)(value+dev_idx+num_accelerator*40), m_device_pool.energy_timestamp(GEOPM_DOMAIN_BOARD_ACCELERATOR, dev_idx, MockLevelZero::M_DOMAIN_ALL));
    }
}
