/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKAPPLICATIONIO_HPP_INCLUDE
#define MOCKAPPLICATIONIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationIO.hpp"

class MockApplicationIO : public geopm::ApplicationIO
{
    public:
        MOCK_METHOD(std::vector<int>, connect, (), (override));
        MOCK_METHOD(bool, do_shutdown, (), (override));
        MOCK_METHOD(std::set<std::string>, region_name_set, (), (const, override));
};

#endif
