/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKENDPOINTUSER_HPP_INCLUDE
#define MOCKENDPOINTUSER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "EndpointUser.hpp"

class MockEndpointUser : public geopm::EndpointUser
{
    public:
        MOCK_METHOD(double, read_policy, (std::vector<double> & policy), (override));
        MOCK_METHOD(void, write_sample, (const std::vector<double> &sample),
                    (override));
};

#endif
