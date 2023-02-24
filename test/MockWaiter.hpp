/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKWAITER_HPP_INCLUDE
#define MOCKWAITER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Waiter.hpp"

class MockWaiter : public geopm::Waiter
{
        MOCK_METHOD(void, reset, (), (override));
        MOCK_METHOD(void, reset, (double period), (override));
        MOCK_METHOD(void, wait, (), (override));
        MOCK_METHOD(double, period, (), (const, override));
};

#endif
