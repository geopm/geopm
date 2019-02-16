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
        void TearDown(void);
        MockPlatformIO m_platform_io;
        std::string m_path = "test.profiletrace";
        std::string m_hostname = "myhost";
};

void ProfileTracerTest::TearDown(void)
{
    unlink(m_path.c_str());
}

TEST_F(ProfileTracerTest, format)
{
    EXPECT_CALL(m_platform_io, read_signal("TIME", geopm::IPlatformTopo::M_DOMAIN_BOARD, 0))
            .WillOnce(Return(5.0));

    {
        struct geopm_time_s time_stamp;
        geopm_time(&time_stamp);
        geopm::ProfileTracer tracer(2, true, m_path, m_platform_io, time_stamp);

        std::vector<std::pair<uint64_t, struct geopm_prof_message_s> > data;

        uint64_t region_id = 0x00000000fa5920d6ULL | GEOPM_REGION_HINT_COMPUTE;
        double progress = 0.0;

        geopm_time_add(&time_stamp, 10, &time_stamp);
        for (int rank = 0; rank != 4; ++rank) {
            data.push_back({region_id, {rank, region_id, time_stamp, progress}});
            geopm_time_add(&time_stamp, 1, &time_stamp);
        }

        geopm_time_add(&time_stamp, 20, &time_stamp);
        progress = 1.0;
        for (int rank = 3; rank != -1; --rank) {
            data.push_back({region_id, {rank, region_id, time_stamp, progress}});
            geopm_time_add(&time_stamp, 1, &time_stamp);
        }
        tracer.update(data.begin(), data.end());
    }
    std::string output = geopm::read_file(m_path);
    std::vector<std::string> output_lines = geopm::split_string(output, "\n");
    std::string expect = "rank|region_hash|region_hint|timestamp|progress\n"
                         "0|0x00000000fa5920d6|0x0000000200000000|1.5000000000000000e+01|0.0000000000000000e+00\n"
                         "1|0x00000000fa5920d6|0x0000000200000000|1.6000000000000000e+01|0.0000000000000000e+00\n"
                         "2|0x00000000fa5920d6|0x0000000200000000|1.7000000000000000e+01|0.0000000000000000e+00\n"
                         "3|0x00000000fa5920d6|0x0000000200000000|1.8000000000000000e+01|0.0000000000000000e+00\n"
                         "3|0x00000000fa5920d6|0x0000000200000000|3.9000000000000000e+01|1.0000000000000000e+00\n"
                         "2|0x00000000fa5920d6|0x0000000200000000|4.0000000000000000e+01|1.0000000000000000e+00\n"
                         "1|0x00000000fa5920d6|0x0000000200000000|4.1000000000000000e+01|1.0000000000000000e+00\n"
                         "0|0x00000000fa5920d6|0x0000000200000000|4.2000000000000000e+01|1.0000000000000000e+00\n";
    std::vector<std::string> expect_lines = geopm::split_string(expect, "\n");
    ASSERT_EQ(expect_lines.size(), output_lines.size());

    auto output_it = output_lines.begin();
    for (const auto &expect_it : expect_lines) {
        EXPECT_EQ(expect_it, *output_it);
        ++output_it;
    }
}
