/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPROFILETRACER_HPP_INCLUDE
#define MOCKPROFILETRACER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ProfileTracer.hpp"

class MockProfileTracer : public geopm::ProfileTracer
{
    public:
        MOCK_METHOD(void, update, (const std::vector<geopm::record_s> &records),
                    (override));
};

#endif
