/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKAPPLICATIONIO_HPP_INCLUDE
#define MOCKAPPLICATIONIO_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ApplicationIO.hpp"

class MockApplicationIO : public geopm::ApplicationIO
{
    public:
        MOCK_METHOD(void, connect, (), (override));
        MOCK_METHOD(bool, do_shutdown, (), (override));
        MOCK_METHOD(std::string, report_name, (), (const, override));
        MOCK_METHOD(std::string, profile_name, (), (const, override));
        MOCK_METHOD(std::set<std::string>, region_name_set, (), (const, override));
        MOCK_METHOD(void, controller_ready, (), (override));
        MOCK_METHOD(void, abort, (), (override));
};

#endif
