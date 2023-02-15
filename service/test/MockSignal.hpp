/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSIGNAL_HPP_INCLUDE
#define MOCKSIGNAL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Signal.hpp"

class MockSignal : public geopm::Signal
{
    public:
        MOCK_METHOD(void, setup_batch, (), (override));
        MOCK_METHOD(double, sample, (), (override));
        MOCK_METHOD(double, read, (), (const, override));
};

#endif
