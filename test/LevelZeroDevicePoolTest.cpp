/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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
#include "Helper.hpp"
#include "Exception.hpp"
#include "MockLevelZeroShim.hpp"
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

        std::shared_ptr<MockLevelZeroShim> m_shim;
};

void LevelZeroDevicePoolTest::SetUp()
{
    m_shim = std::make_shared<MockLevelZeroShim>();
}

void LevelZeroDevicePoolTest::TearDown()
{
}

TEST_F(LevelZeroDevicePoolTest, device_count)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;
    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_shim, num_accelerator_subdevice()).WillRepeatedly(Return(num_accelerator_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_shim);

    EXPECT_EQ(num_accelerator, m_device_pool.num_accelerator());
    EXPECT_EQ(num_accelerator_subdevice, m_device_pool.num_accelerator_subdevice());
}

TEST_F(LevelZeroDevicePoolTest, subdevice_conversion_and_function)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;
    const int num_subdevice_per_device = num_accelerator_subdevice/num_accelerator;

    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_shim, num_accelerator_subdevice()).WillRepeatedly(Return(num_accelerator_subdevice));

    int value = 1500;
    int offset = 0;
    int domain_count = 1; //any non-zero number to ensure we don't throw
    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_CALL(*m_shim, frequency_domain_count(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        EXPECT_CALL(*m_shim, engine_domain_count(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        for (int sub_idx = 0; sub_idx < num_subdevice_per_device; ++sub_idx) {
            EXPECT_CALL(*m_shim, frequency_status(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset));
            EXPECT_CALL(*m_shim, frequency_min(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*10));
            EXPECT_CALL(*m_shim, frequency_max(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*20));

            EXPECT_CALL(*m_shim, active_time(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*30));
            EXPECT_CALL(*m_shim, active_time_timestamp(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset+num_accelerator_subdevice*40));
            ++offset;
        }
    }
    LevelZeroDevicePoolImp m_device_pool(*m_shim);

    for (int sub_idx = 0; sub_idx < num_accelerator_subdevice; ++sub_idx) {
        EXPECT_EQ(value+sub_idx, m_device_pool.frequency_status(sub_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_accelerator_subdevice*10, m_device_pool.frequency_min(sub_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE));
        EXPECT_EQ(value+sub_idx+num_accelerator_subdevice*20, m_device_pool.frequency_max(sub_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE));

        EXPECT_EQ((uint64_t)(value+sub_idx+num_accelerator_subdevice*30), m_device_pool.active_time(sub_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE));
        EXPECT_EQ((uint64_t)(value+sub_idx+num_accelerator_subdevice*40), m_device_pool.active_time_timestamp(sub_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE));

        EXPECT_NO_THROW(m_device_pool.frequency_control(sub_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE,value));
    }
}

TEST_F(LevelZeroDevicePoolTest, subdevice_conversion_error)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 9;

    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_shim, num_accelerator_subdevice()).WillRepeatedly(Return(num_accelerator_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_shim);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(0, GEOPM_LEVELZERO_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "GEOPM Requires the number of subdevices to be evenly divisible by the number of devices");
}

TEST_F(LevelZeroDevicePoolTest, domain_error)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;
    const int num_subdevice_per_device = num_accelerator_subdevice/num_accelerator;

    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_shim, num_accelerator_subdevice()).WillRepeatedly(Return(num_accelerator_subdevice));

    int value = 1500;
    int offset = 0;
    int domain_count = 0; //zero to cause a throw
    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_CALL(*m_shim, frequency_domain_count(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE)).WillRepeatedly(Return(domain_count));
        for (int sub_idx = 0; sub_idx < num_subdevice_per_device; ++sub_idx) {
            EXPECT_CALL(*m_shim, frequency_status(dev_idx, GEOPM_LEVELZERO_DOMAIN_COMPUTE, sub_idx)).WillOnce(Return(value+offset));
            ++offset;
        }
    }
    LevelZeroDevicePoolImp m_device_pool(*m_shim);

    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(0, GEOPM_LEVELZERO_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "Not supported on this hardware");
}

TEST_F(LevelZeroDevicePoolTest, subdevice_range_check)
{
    const int num_accelerator = 4;
    const int num_accelerator_subdevice = 8;

    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));
    EXPECT_CALL(*m_shim, num_accelerator_subdevice()).WillRepeatedly(Return(num_accelerator_subdevice));

    LevelZeroDevicePoolImp m_device_pool(*m_shim);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.frequency_status(num_accelerator_subdevice, GEOPM_LEVELZERO_DOMAIN_COMPUTE), GEOPM_ERROR_INVALID, "subdevice idx "+std::to_string(num_accelerator_subdevice)+" is out of range");
}

TEST_F(LevelZeroDevicePoolTest, device_range_check)
{
    const int num_accelerator = 4;

    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));

    LevelZeroDevicePoolImp m_device_pool(*m_shim);
    GEOPM_EXPECT_THROW_MESSAGE(m_device_pool.energy(num_accelerator), GEOPM_ERROR_INVALID, "device idx "+std::to_string(num_accelerator)+" is out of range");
}

TEST_F(LevelZeroDevicePoolTest, device_function_check)
{
    const int num_accelerator = 4;

    EXPECT_CALL(*m_shim, num_accelerator()).WillRepeatedly(Return(num_accelerator));

    int value = 1500;
    int offset = 0;
    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_CALL(*m_shim, power_limit_tdp(dev_idx)).WillOnce(Return(value+offset));
        EXPECT_CALL(*m_shim, power_limit_min(dev_idx)).WillOnce(Return(value+offset+num_accelerator*10));
        EXPECT_CALL(*m_shim, power_limit_max(dev_idx)).WillOnce(Return(value+offset+num_accelerator*20));
        EXPECT_CALL(*m_shim, energy(dev_idx)).WillOnce(Return(value+offset+num_accelerator*30));
        EXPECT_CALL(*m_shim, energy_timestamp(dev_idx)).WillOnce(Return(value+offset+num_accelerator*40));
        ++offset;
    }
    LevelZeroDevicePoolImp m_device_pool(*m_shim);

    for (int dev_idx = 0; dev_idx < num_accelerator; ++dev_idx) {
        EXPECT_EQ(value+dev_idx, m_device_pool.power_limit_tdp(dev_idx));
        EXPECT_EQ(value+dev_idx+num_accelerator*10, m_device_pool.power_limit_min(dev_idx));
        EXPECT_EQ(value+dev_idx+num_accelerator*20, m_device_pool.power_limit_max(dev_idx));
        EXPECT_EQ((uint64_t)(value+dev_idx+num_accelerator*30), m_device_pool.energy(dev_idx));
        EXPECT_EQ((uint64_t)(value+dev_idx+num_accelerator*40), m_device_pool.energy_timestamp(dev_idx));
    }
}
