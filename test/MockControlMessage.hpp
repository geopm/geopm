/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKCONTROLMESSAGE_HPP_INCLUDE
#define MOCKCONTROLMESSAGE_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ControlMessage.hpp"

class MockControlMessage : public geopm::ControlMessage
{
    public:
        MOCK_METHOD(void, step, (), (override));
        MOCK_METHOD(void, wait, (), (override));
        MOCK_METHOD(void, abort, (), (override));
        MOCK_METHOD(void, cpu_rank, (int cpu_idx, int rank), (override));
        MOCK_METHOD(int, cpu_rank, (int cpu_idx), (const, override));
        MOCK_METHOD(bool, is_sample_begin, (), (const, override));
        MOCK_METHOD(bool, is_sample_end, (), (const, override));
        MOCK_METHOD(bool, is_name_begin, (), (const, override));
        MOCK_METHOD(bool, is_shutdown, (), (const, override));
        MOCK_METHOD(void, loop_begin, (), (override));
};

#endif
