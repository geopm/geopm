/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
