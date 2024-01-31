/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSSTCLOSGOVERNOR_HPP_INCLUDE
#define MOCKSSTCLOSGOVERNOR_HPP_INCLUDE

#include "gmock/gmock.h"

#include "SSTClosGovernor.hpp"

class MockSSTClosGovernor : public geopm::SSTClosGovernor
{
    public:
        MOCK_METHOD(void, init_platform_io, (), (override));
        MOCK_METHOD(int, clos_domain_type, (), (const, override));
        MOCK_METHOD(void, adjust_platform,
                    (const std::vector<double> &clos_by_core), (override));
        MOCK_METHOD(bool, do_write_batch, (), (const, override));
        MOCK_METHOD(void, enable_sst_turbo_prioritization, (), (override));
        MOCK_METHOD(void, disable_sst_turbo_prioritization, (), (override));
};

#endif // MOCKSSTCLOSGOVERNOR_HPP_INCLUDE
