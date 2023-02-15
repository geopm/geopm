/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKCONTROL_HPP_INCLUDE
#define MOCKCONTROL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Control.hpp"

class MockControl : public geopm::Control
{
    public:
        MOCK_METHOD(void, setup_batch, (), (override));
        MOCK_METHOD(void, adjust, (double value), (override));
        MOCK_METHOD(void, write, (double value), (override));
        MOCK_METHOD(void, save, (), (override));
        MOCK_METHOD(void, restore, (), (override));
};

#endif
