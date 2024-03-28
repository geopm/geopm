/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKDCGMDEVICEPOOL_HPP_INCLUDE
#define MOCKDCGMDEVICEPOOL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "DCGMDevicePool.hpp"

class MockDCGMDevicePool : public geopm::DCGMDevicePool
{
    public:
        MOCK_METHOD(int, num_device, (), (const, override));
        MOCK_METHOD(double, sample, (int, int), (const, override));
        MOCK_METHOD(void, update, (int), (override));
        MOCK_METHOD(void, update_rate, (int), (override));
        MOCK_METHOD(void, max_storage_time, (int), (override));
        MOCK_METHOD(void, max_samples, (int), (override));
        MOCK_METHOD(void, polling_enable, (), (override));
        MOCK_METHOD(void, polling_disable, (), (override));
};

#endif
