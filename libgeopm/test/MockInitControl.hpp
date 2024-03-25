/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKINITCONTROL_HPP_INCLUDE
#define MOCKINITCONTROL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "InitControl.hpp"

class MockInitControl : public geopm::InitControl
{
    public:
        MOCK_METHOD(void, parse_input, (const std::string &input_file), (override));
        MOCK_METHOD(void, write_controls, (), (const, override));
};

#endif
