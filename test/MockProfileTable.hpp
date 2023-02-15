/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKPROFILETABLE_HPP_INCLUDE
#define MOCKPROFILETABLE_HPP_INCLUDE

#include "gmock/gmock.h"

#include "ProfileTable.hpp"

class MockProfileTable : public geopm::ProfileTable
{
    public:
        MOCK_METHOD(uint64_t, key, (const std::string &name), (override));
        MOCK_METHOD(void, insert, (const struct geopm_prof_message_s &value),
                    (override));
        MOCK_METHOD(size_t, capacity, (), (const, override));
        MOCK_METHOD(size_t, size, (), (const, override));
        MOCK_METHOD(void, dump,
                    ((std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator)content,
                     size_t &length),
                    (override));
        MOCK_METHOD(bool, name_fill, (size_t header_offset), (override));
        MOCK_METHOD(bool, name_set,
                    (size_t header_offset, std::set<std::string> &name), (override));
};

#endif
