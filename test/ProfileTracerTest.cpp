/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <memory>
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ProfileTracerImp.hpp"
#include "geopm/Helper.hpp"
#include "record.hpp"
#include "geopm_prof.h"
#include "geopm_time.h"
#include "geopm_hint.h"
#include "MockApplicationSampler.hpp"

using testing::Return;
using geopm::ProfileTracer;
using geopm::ProfileTracerImp;
using geopm::record_s;

class ProfileTracerTest : public ::testing::Test
{
    protected:
        void SetUp(void);
        std::string m_start_time = "Mon Sep 14 19:00:25 2020";
        std::string m_path = "test.profiletrace";
        std::string m_host_name = "myhost";
#ifdef GEOPM_ENABLE_MPI
        std::string m_output_path = m_path + "-" + m_host_name;
#else
        std::string m_output_path = m_path;
#endif
        std::vector<record_s> m_data;
        MockApplicationSampler m_application_sampler;
};

void ProfileTracerTest::SetUp(void)
{
    uint64_t region_hash = 0x00000000fa5920d6ULL;
    geopm_time_s time = {{10, 0}};
    int event = geopm::EVENT_REGION_ENTRY;
    for (int rank = 0; rank != 4; ++rank) {
        m_data.push_back({time, rank, event, region_hash});
        time.t.tv_sec += 1;
    }

    time.t.tv_sec += 20;
    event = geopm::EVENT_REGION_EXIT;
    for (int rank = 3; rank != -1; --rank) {
        m_data.push_back({time, rank, event, region_hash});
        time.t.tv_sec += 1;
    }

     m_data.push_back({{{40, 0}}, 0, geopm::EVENT_SHORT_REGION, 88});
     m_data.push_back({{{41, 0}}, 1, geopm::EVENT_EPOCH_COUNT, 1});
}

TEST_F(ProfileTracerTest, construct_update_destruct)
{
    EXPECT_CALL(m_application_sampler, get_short_region(88))
        .WillOnce(Return(geopm::short_region_s{
            0xdeadbeef, 2, 3.14
        }));

    {
        // Test that the constructor and update methods do not throw
        std::unique_ptr<ProfileTracer> tracer = geopm::make_unique<ProfileTracerImp>(
            m_start_time, geopm_time_s {{0, 0}}, 2, true, m_path, "", m_application_sampler);
        tracer->update(m_data);
    }
    // Test that a file was created by deleting it without error
    int err = unlink(m_path.c_str());
    EXPECT_EQ(0, err);
}

TEST_F(ProfileTracerTest, format)
{
    EXPECT_CALL(m_application_sampler, get_short_region(88))
        .WillOnce(Return(geopm::short_region_s{
            0xdeadbeef, 2, 3.14
        }));

    {
        std::unique_ptr<ProfileTracer> tracer = geopm::make_unique<ProfileTracerImp>(
            m_start_time, geopm_time_s {{0, 0}}, 2, true, m_path, m_host_name, m_application_sampler);
        tracer->update(m_data);
    }

    std::string output = geopm::read_file(m_output_path);
    std::vector<std::string> output_lines = geopm::string_split(output, "\n");
    std::vector<std::string> expect_lines = {
        "TIME|PROCESS|EVENT|SIGNAL",
        "10|0|REGION_ENTRY|0xfa5920d6",
        "11|1|REGION_ENTRY|0xfa5920d6",
        "12|2|REGION_ENTRY|0xfa5920d6",
        "13|3|REGION_ENTRY|0xfa5920d6",
        "34|3|REGION_EXIT|0xfa5920d6",
        "35|2|REGION_EXIT|0xfa5920d6",
        "36|1|REGION_EXIT|0xfa5920d6",
        "37|0|REGION_EXIT|0xfa5920d6",
        "40|0|EVENT_SHORT_REGION|0xdeadbeef",
        "41|1|EPOCH_COUNT|1"
    };
    auto expect_it = expect_lines.begin();
    for (const auto &output_it : output_lines) {
        if (expect_it != expect_lines.end() &&
            !output_it.empty() &&
            output_it[0] != '#') {
            EXPECT_EQ(*expect_it, output_it);
            ++expect_it;
        }
    }
    EXPECT_EQ(expect_lines.end(), expect_it);
    int err = unlink(m_output_path.c_str());
    EXPECT_EQ(0, err);
}
