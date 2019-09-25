/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include "ProfileTracer.hpp"
#include "MockPlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Helper.hpp"
#include "geopm.h"
#include "geopm_time.h"
#include "geopm_internal.h"

using testing::Return;

class ProfileTracerTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        MockPlatformIO m_platform_io;
        struct geopm_time_s m_time_stamp;
        std::string m_path = "test.profiletrace";
        std::string m_host_name = "myhost";
        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > m_data;
};

void ProfileTracerTest::SetUp(void)
{
    uint64_t region_id = 0x00000000fa5920d6ULL | GEOPM_REGION_HINT_COMPUTE;
    double progress = 0.0;

    struct geopm_time_s time_stamp;
    geopm_time(&m_time_stamp);
    time_stamp = m_time_stamp;
    geopm_time_add(&time_stamp, 10, &time_stamp);
    for (int rank = 0; rank != 4; ++rank) {
        m_data.push_back({region_id, {rank, region_id, time_stamp, progress}});
        geopm_time_add(&time_stamp, 1, &time_stamp);
    }

    geopm_time_add(&time_stamp, 20, &time_stamp);
    progress = 1.0;
    for (int rank = 3; rank != -1; --rank) {
        m_data.push_back({region_id, {rank, region_id, time_stamp, progress}});
        geopm_time_add(&time_stamp, 1, &time_stamp);
    }
}

TEST_F(ProfileTracerTest, construct_update_destruct)
{
    // Test that the tracer samples time
    EXPECT_CALL(m_platform_io, read_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
            .WillOnce(Return(5.0));
    {
        // Test that the constructor and update methods do not throw
        std::unique_ptr<geopm::ProfileTracer> tracer = geopm::make_unique<geopm::ProfileTracerImp>(2, true, m_path, "", m_platform_io, GEOPM_TIME_REF);
        tracer->update(m_data.begin(), m_data.end());
    }
    // Test that a file was created by deleting it without error
    int err = unlink(m_path.c_str());
    EXPECT_EQ(0, err);
}

TEST_F(ProfileTracerTest, format)
{
    EXPECT_CALL(m_platform_io, read_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
            .WillOnce(Return(5.0));
    {
        geopm::ProfileTracerImp tracer(2, true, m_path, m_host_name, m_platform_io, m_time_stamp);
        tracer.update(m_data.begin(), m_data.end());
    }
    std::string output_path = m_path + "-" + m_host_name;
    std::string output = geopm::read_file(output_path);
    std::vector<std::string> output_lines = geopm::string_split(output, "\n");
    std::vector<std::string> expect_lines = {
        "RANK|REGION_HASH|REGION_HINT|TIMESTAMP|PROGRESS",
        "0|0x00000000fa5920d6|0x0000000200000000|15|0",
        "1|0x00000000fa5920d6|0x0000000200000000|16|0",
        "2|0x00000000fa5920d6|0x0000000200000000|17|0",
        "3|0x00000000fa5920d6|0x0000000200000000|18|0",
        "3|0x00000000fa5920d6|0x0000000200000000|39|1",
        "2|0x00000000fa5920d6|0x0000000200000000|40|1",
        "1|0x00000000fa5920d6|0x0000000200000000|41|1",
        "0|0x00000000fa5920d6|0x0000000200000000|42|1"
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
