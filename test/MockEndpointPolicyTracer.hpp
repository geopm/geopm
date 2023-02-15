/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKENDPOINTPOLICYTRACER_HPP_INCLUDE
#define MOCKENDPOINTPOLICYTRACER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "EndpointPolicyTracer.hpp"

class MockEndpointPolicyTracer : public geopm::EndpointPolicyTracer
{
    public:
        MOCK_METHOD(void, update, (const std::vector<double> &policy), (override));
};

#endif
