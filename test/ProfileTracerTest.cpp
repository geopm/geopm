/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include <memory>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ProfileTracerImp.hpp"
#include "Helper.hpp"
#include "record.hpp"
#include "geopm.h"
#include "geopm_time.h"
#include "geopm_internal.h"

using testing::Return;
using geopm::ProfileTracer;
using geopm::ProfileTracerImp;
using geopm::record_s;

class ProfileTracerTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        struct geopm_time_s m_time_stamp;
        std::string m_path = "test.profiletrace";
        std::string m_host_name = "myhost";
        std::vector<record_s> m_data;
};

void ProfileTracerTest::SetUp(void)
{
    uint64_t region_hash = 0x00000000fa5920d6ULL;
    double time = 10.0;
    int event = geopm::EVENT_REGION_ENTRY;
    for (int rank = 0; rank != 4; ++rank) {
        m_data.push_back({time, rank, event, region_hash});
        time += 1.0;
    }

    time += 20;
    event = geopm::EVENT_REGION_EXIT;
    for (int rank = 3; rank != -1; --rank) {
        m_data.push_back({time, rank, event, region_hash});
        time += 1.0;
    }
}

TEST_F(ProfileTracerTest, construct_update_destruct)
{
    {
        // Test that the constructor and update methods do not throw
        std::unique_ptr<ProfileTracer> tracer = geopm::make_unique<ProfileTracerImp>(2, true, m_path, "", geopm::time_zero());
        tracer->update(m_data);
    }
    // Test that a file was created by deleting it without error
    int err = unlink(m_path.c_str());
    EXPECT_EQ(0, err);
}

TEST_F(ProfileTracerTest, format)
{
    {
        std::unique_ptr<ProfileTracer> tracer = geopm::make_unique<ProfileTracerImp>(2, true, m_path, m_host_name, m_time_stamp);
        tracer->update(m_data);
    }
    std::string output_path = m_path + "-" + m_host_name;
    std::string output = geopm::read_file(output_path);
    std::vector<std::string> output_lines = geopm::string_split(output, "\n");
    std::vector<std::string> expect_lines = {
        "TIME|PROCESS|EVENT|SIGNAL",
        "10|0|REGION_ENTRY|0x00000000fa5920d6",
        "11|1|REGION_ENTRY|0x00000000fa5920d6",
        "12|2|REGION_ENTRY|0x00000000fa5920d6",
        "13|3|REGION_ENTRY|0x00000000fa5920d6",
        "34|3|REGION_EXIT|0x00000000fa5920d6",
        "35|2|REGION_EXIT|0x00000000fa5920d6",
        "36|1|REGION_EXIT|0x00000000fa5920d6",
        "37|0|REGION_EXIT|0x00000000fa5920d6",
    };
    auto expect_it = expect_lines.begin();
    for (const auto &output_it : output_lines) {
        if (output_it[0] != '#' && output_it.size()) {
            EXPECT_EQ(*expect_it, output_it);
            ++expect_it;
        }
    }
    int err = unlink(output_path.c_str());
    EXPECT_EQ(0, err);
}
