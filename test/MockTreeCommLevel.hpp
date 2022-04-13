/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKTREECOMMLEVEL_HPP_INCLUDE
#define MOCKTREECOMMLEVEL_HPP_INCLUDE

#include "gmock/gmock.h"

#include "TreeCommLevel.hpp"

class MockTreeCommLevel : public geopm::TreeCommLevel
{
    public:
        MOCK_METHOD(int, level_rank, (), (const, override));
        MOCK_METHOD(void, send_up, (const std::vector<double> &sample), (override));
        MOCK_METHOD(void, send_down,
                    (const std::vector<std::vector<double> > &policy), (override));
        MOCK_METHOD(bool, receive_up,
                    (std::vector<std::vector<double> > & sample), (override));
        MOCK_METHOD(bool, receive_down, (std::vector<double> & policy), (override));
        MOCK_METHOD(size_t, overhead_send, (), (const, override));
};

#endif
