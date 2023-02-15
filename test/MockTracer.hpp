/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKTRACER_HPP_INCLUDE
#define MOCKTRACER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Tracer.hpp"
#include "geopm_prof.h"

class MockTracer : public geopm::Tracer
{
    public:
        MOCK_METHOD(void, columns,
                    (const std::vector<std::string> &agent_cols,
                     const std::vector<std::function<std::string(double)> > &agent_formats),
                    (override));
        MOCK_METHOD(void, update, (const std::vector<double> &agent_vals), (override));
        MOCK_METHOD(void, flush, (), (override));
};

#endif
