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

#ifndef MOCKPROFILESAMPLER_HPP_INCLUDE
#define MOCKPROFILESAMPLER_HPP_INCLUDE

#include "gmock/gmock.h"

#include "Comm.hpp"
#include "ProfileSampler.hpp"

class MockProfileSampler : public geopm::ProfileSampler
{
    public:
        MOCK_METHOD(bool, do_shutdown, (), (const, override));
        MOCK_METHOD(bool, do_report, (), (const, override));
        MOCK_METHOD(void, region_names, (), (override));
        MOCK_METHOD(void, initialize, (), (override));
        MOCK_METHOD(int, rank_per_node, (), (const, override));
        MOCK_METHOD(std::vector<int>, cpu_rank, (), (const, override));
        MOCK_METHOD(std::set<std::string>, name_set, (), (const, override));
        MOCK_METHOD(std::string, report_name, (), (const, override));
        MOCK_METHOD(std::string, profile_name, (), (const, override));
        MOCK_METHOD(void, controller_ready, (), (override));
        MOCK_METHOD(void, abort, (), (override));
        MOCK_METHOD(void, check_sample_end, (), (override));
};

#endif
