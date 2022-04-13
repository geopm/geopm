/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKRECORDFILTER_HPP_INCLUDE
#define MOCKRECORDFILTER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "RecordFilter.hpp"

class MockRecordFilter : public geopm::RecordFilter
{
    public:
        MOCK_METHOD(std::vector<geopm::record_s>, filter,
                    (const geopm::record_s &record), (override));
};

#endif
