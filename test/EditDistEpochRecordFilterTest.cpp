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

#include <cstdint>
#include <iostream>
#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "EditDistEpochRecordFilter.hpp"
#include "MockEditDistPeriodicityDetector.hpp"
#include "MockApplicationSampler.hpp"
#include "record.hpp"
#include "Helper.hpp"

using geopm::EditDistEpochRecordFilter;
using geopm::record_s;

void check_vals(std::vector<record_s> testout, std::vector<int> epoch_time_vector);
std::vector<record_s> filter_file(std::string trace_file_path, int history_size, bool debug=false);
void print_records(std::vector<record_s> recs, int filter=-1);
std::vector<int> extract_epoch_times(std::vector<record_s> recs);

class EditDistEpochRecordFilterTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::vector<int> m_in_events;
        std::vector<int> m_out_events;
        std::string m_test_root_path;
};

void EditDistEpochRecordFilterTest::SetUp()
{
    m_in_events = {
        geopm::EVENT_HINT,
        geopm::EVENT_REGION_ENTRY,
        geopm::EVENT_REGION_EXIT,
        geopm::EVENT_PROFILE,
        geopm::EVENT_REPORT,
        geopm::EVENT_CLAIM_CPU,
        geopm::EVENT_RELEASE_CPU,
        geopm::EVENT_NAME_KEY,
    };
    m_out_events = {
        geopm::EVENT_EPOCH_COUNT,
    };
    m_test_root_path = "./test/";
}

/*
    Only one region (hash: A) is repeated over and over again.
    Test code sends region entry and exit events to the filter. They are expected to pass through.
    First 3 iterations, the epoch count events are ignored (for ramp up).
    Then, for 10 iterations repoch count is expected.

    Filter size: 16
 */
TEST_F(EditDistEpochRecordFilterTest, one_region_repeated)
{
    uint64_t hash = 0xAULL;
    record_s record {
        0.0,
        0,
        geopm::EVENT_REGION_ENTRY,
        hash
    };

    double time = 0.0;
    geopm::EditDistEpochRecordFilter ederf(16);
    for (uint64_t count = 0; count <= 4; ++count) {

        std::vector<record_s> result;

        record.time = time;
        record.event = geopm::EVENT_REGION_ENTRY;
        result = ederf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(time, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);

        time += 1.0;

        record.time = time;
        record.event = geopm::EVENT_REGION_EXIT;
        result = ederf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(time, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[0].event);
        EXPECT_EQ(hash, result[0].signal);

        time += 1.0;
    }

    for (uint64_t count = 1; count <= 10; ++count) {
        record.time = time;
        record.event = geopm::EVENT_REGION_ENTRY;
        std::vector<record_s> result = ederf.filter(record);

        EXPECT_EQ(2ULL, result.size());

        EXPECT_EQ(time, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);

        EXPECT_EQ(time, result[1].time);
        EXPECT_EQ(0, result[1].process);
        EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
        EXPECT_EQ(count, result[1].signal);

        time += 1.0;

        record.time = time;
        record.event = geopm::EVENT_REGION_EXIT;
        result = ederf.filter(record);

        ASSERT_EQ(1ULL, result.size());

        EXPECT_EQ(time, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
    }
}

TEST_F(EditDistEpochRecordFilterTest, filter_in)
{
    record_s record {};
    geopm::EditDistEpochRecordFilter ederf(16);
    for (auto event : m_in_events) {
        record.event = event;
        std::vector<record_s> result = ederf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(event, result[0].event);
        EXPECT_EQ(0ULL, result[0].signal);
    }
}

TEST_F(EditDistEpochRecordFilterTest, filter_out)
{
    record_s record {};
    std::vector<record_s> result;
    geopm::EditDistEpochRecordFilter ederf(16);
    for (auto event : m_out_events) {
        record.event = event;
        result = ederf.filter(record);
        EXPECT_EQ(0ULL, result.size());
    }
}

/// TESTED FROM TRACE FILES

/// Pattern 0: (A)x10
TEST_F(EditDistEpochRecordFilterTest, pattern_a)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.0_pattern_a.trace", history_size);
    check_vals(testout, {5, 6, 7, 8, 9});
}

/// Pattern 1: (AB)x15
TEST_F(EditDistEpochRecordFilterTest, pattern_ab)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.1_pattern_ab.trace", history_size);
    check_vals(testout, {7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29});
}

/// Pattern 2: (ABB)x12
TEST_F(EditDistEpochRecordFilterTest, pattern_abb)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.2_pattern_abb.trace",history_size);
    check_vals(testout, {9, 12, 15, 18, 21, 24, 27, 30, 33});
}

/// Pattern 3: (ABCDABCDABCDC) (ABCDABCDABCDABCDC)x6 (ABCD)
TEST_F(EditDistEpochRecordFilterTest, pattern_abcdc)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.3_pattern_abcdc.trace", history_size);
    check_vals(testout, {11, 16, 20, 24, 28, 45, 71, 88, 105});
}

/// Pattern 4: (AB) (ABABC)x3
TEST_F(EditDistEpochRecordFilterTest, pattern_ababc)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.4_pattern_ababc.trace", history_size);
    check_vals(testout, {16, 21, 26, 31});
}

/// Pattern 5: (ABABABC)x6
TEST_F(EditDistEpochRecordFilterTest, pattern_abababc)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.5_pattern_abababc.trace", history_size);
    check_vals(testout, {20, 27, 34, 41});
}

/// Pattern 6: (ABCD)x6 (E) (ABCD)x6
TEST_F(EditDistEpochRecordFilterTest, pattern_add1)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.6_pattern_add1.trace", history_size);
    check_vals(testout, {11, 15, 19, 23, 32, 36, 40, 44, 48});
}

/// Pattern 7: (ABCD)x6 (EF) (ABCD)x9
TEST_F(EditDistEpochRecordFilterTest, pattern_add2)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.7_pattern_add2.trace", history_size);
    check_vals(testout, {11, 15, 19, 23, 32, 36, 40, 44, 48, 52, 56, 60});
}

/// Pattern 8: (ABCD)x6 (ABC) (ABCD)x12
TEST_F(EditDistEpochRecordFilterTest, pattern_subtract1)
{
    int history_size = 100;

    std::vector<record_s> testout = filter_file(m_test_root_path + "EditDistPeriodicityDetectorTest.8_pattern_subtract1.trace", history_size);
    check_vals(testout, {11, 15, 19, 23, 33, 38, 42, 46, 50, 54, 58, 62, 66, 70, 74});
}

std::vector<record_s> filter_file(std::string trace_file_path, int history_size, bool debug)
{
    MockApplicationSampler app;
    app.inject_records(geopm::read_file(trace_file_path));

    geopm::EditDistEpochRecordFilter ederf(history_size);

    std::vector<record_s> recs = app.get_records();

    std::vector<record_s> results;

    for(const auto &it : recs) {
        std::vector<record_s> result = ederf.filter(it);

        if(result.size() > 0) {
            results.insert(results.end(), result.begin(), result.end());
        }
    }
    return results;
}

/// HELPER FUNCTIONS

void check_vals(std::vector<record_s> testout, std::vector<int> epoch_time_vector)
{
    // print_records(testout, geopm::EVENT_EPOCH_COUNT);
    EXPECT_EQ(epoch_time_vector, extract_epoch_times(testout));
}

void print_records(std::vector<record_s> recs, int filter)
{
    for(const auto &rec : recs) {
        if(filter != -1 && rec.event != filter) {
            continue;
        }
        std::cout << "Time=" << rec.time << " Event=" << rec.event << " Signal=" << rec.signal << "\n";
    }
}

std::vector<int> extract_epoch_times(std::vector<record_s> recs)
{
    std::vector<int> results;
    for(const auto &rec : recs) {
        if(rec.event == geopm::EVENT_EPOCH_COUNT) {
            results.push_back(rec.time);
        }
    }
    return results;
}
