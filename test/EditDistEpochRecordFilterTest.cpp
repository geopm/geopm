/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <cstdint>

#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "EditDistEpochRecordFilter.hpp"
#include "MockApplicationSampler.hpp"
#include "record.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

using geopm::EditDistEpochRecordFilter;
using geopm::record_s;

void check_vals(std::vector<record_s> testout, std::vector<int> epoch_time_vector);
std::vector<int> extract_epoch_times(std::vector<record_s> recs);

class EditDistEpochRecordFilterTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::vector<record_s> filter_file(std::string trace_file_path, int history_size, int min_detectable_period);
        std::vector<record_s> filter_file(std::string trace_file_path, int history_size);

        std::vector<int> m_in_events;
        std::vector<int> m_out_events;
        std::string m_trace_file_prefix;
        int m_min_hysteresis_base_period = 4;
        int m_min_detectable_period = 1;
        double m_stable_hyst = 1;
        double m_unstable_hyst = 1.5;

};

void EditDistEpochRecordFilterTest::SetUp()
{
    m_in_events = {
        geopm::EVENT_REGION_ENTRY,
        geopm::EVENT_REGION_EXIT,
        geopm::EVENT_SHORT_REGION,
        geopm::EVENT_AFFINITY,
    };
    m_out_events = {
        geopm::EVENT_EPOCH_COUNT,
    };
    m_trace_file_prefix = GEOPM_SOURCE_DIR;
    m_trace_file_prefix += "/test/EditDistPeriodicityDetectorTest.";
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
        {{0, 0}},
        0,
        geopm::EVENT_REGION_ENTRY,
        hash
    };

    time_t time = 0;
    int buffer_size = 16;
    geopm::EditDistEpochRecordFilter ederf(buffer_size,
                                           m_min_hysteresis_base_period,
                                           m_min_detectable_period,
                                           m_stable_hyst,
                                           m_unstable_hyst);
    for (uint64_t count = 0; count <= 4; ++count) {

        std::vector<record_s> result;

        record.time.t.tv_sec = time;
        record.time.t.tv_nsec = 0;
        record.event = geopm::EVENT_REGION_ENTRY;
        result = ederf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(time, result[0].time.t.tv_sec);
        EXPECT_EQ(0, result[0].time.t.tv_nsec);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);

        time += 1;

        record.time.t.tv_sec = time;
        record.event = geopm::EVENT_REGION_EXIT;
        result = ederf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(time, result[0].time.t.tv_sec);
        EXPECT_EQ(0, result[0].time.t.tv_nsec);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[0].event);
        EXPECT_EQ(hash, result[0].signal);

        time += 1;
    }

    for (uint64_t count = 1; count <= 10; ++count) {
        record.time.t.tv_sec = time;
        record.event = geopm::EVENT_REGION_ENTRY;
        std::vector<record_s> result = ederf.filter(record);

        EXPECT_EQ(2ULL, result.size());

        EXPECT_EQ(time, result[0].time.t.tv_sec);
        EXPECT_EQ(0, result[0].time.t.tv_nsec);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);

        EXPECT_EQ(time, result[1].time.t.tv_sec);
        EXPECT_EQ(0, result[1].time.t.tv_nsec);
        EXPECT_EQ(0, result[1].process);
        EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
        EXPECT_EQ(count, result[1].signal);

        time += 1;

        record.time.t.tv_sec = time;
        record.event = geopm::EVENT_REGION_EXIT;
        result = ederf.filter(record);

        ASSERT_EQ(1ULL, result.size());

        EXPECT_EQ(time, result[0].time.t.tv_sec);
        EXPECT_EQ(0, result[0].time.t.tv_nsec);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
    }
}

TEST_F(EditDistEpochRecordFilterTest, filter_in)
{
    record_s record {};
    int buffer_size = 16;
    geopm::EditDistEpochRecordFilter ederf(buffer_size,
                                           m_min_hysteresis_base_period,
                                           m_min_detectable_period,
                                           m_stable_hyst,
                                           m_unstable_hyst);
    for (auto event : m_in_events) {
        record.event = event;
        std::vector<record_s> result = ederf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(0, result[0].time.t.tv_sec);
        EXPECT_EQ(0, result[0].time.t.tv_nsec);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(event, result[0].event);
        EXPECT_EQ(0ULL, result[0].signal);
    }
}

TEST_F(EditDistEpochRecordFilterTest, filter_out)
{
    record_s record {};
    std::vector<record_s> result;
    int buffer_size = 16;
    geopm::EditDistEpochRecordFilter ederf(buffer_size,
                                           m_min_hysteresis_base_period,
                                           m_min_detectable_period,
                                           m_stable_hyst,
                                           m_unstable_hyst);
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
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "0_pattern_a.trace", history_size);
    check_vals(testout, {5, 6, 7, 8, 9});
}

/// Pattern 1: (AB)x15
TEST_F(EditDistEpochRecordFilterTest, pattern_ab)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "1_pattern_ab.trace", history_size);
    check_vals(testout, {7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29});
}

/// Pattern 2: (ABB)x12
TEST_F(EditDistEpochRecordFilterTest, pattern_abb)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "2_pattern_abb.trace",history_size);
    check_vals(testout, {9, 12, 15, 18, 21, 24, 27, 30, 33});
}

/// Pattern 3: (ABCDABCDABCDC) (ABCDABCDABCDABCDC)x6 (ABCD)
TEST_F(EditDistEpochRecordFilterTest, pattern_abcdc)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "3_pattern_abcdc.trace", history_size);
    check_vals(testout, {11, 16, 24, 28, 52, 69, 86, 103});
}

/// Pattern 4: (AB) (ABABC)x3
TEST_F(EditDistEpochRecordFilterTest, pattern_ababc)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "4_pattern_ababc.trace", history_size);
    check_vals(testout, {16, 21, 26, 31});
}

/// Pattern 5: (ABABABC)x6
TEST_F(EditDistEpochRecordFilterTest, pattern_abababc)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "5_pattern_abababc.trace", history_size);
    check_vals(testout, {20, 27, 34, 41});
}

/// Pattern 6: (ABCD)x6 (E) (ABCD)x6
TEST_F(EditDistEpochRecordFilterTest, pattern_add1)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "6_pattern_add1.trace", history_size);
    check_vals(testout, {11, 15, 19, 23, 36, 40, 44, 48});
}

/// Pattern 7: (ABCD)x6 (EF) (ABCD)x9
TEST_F(EditDistEpochRecordFilterTest, pattern_add2)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "7_pattern_add2.trace", history_size);
    check_vals(testout, {11, 15, 19, 23, 36, 40, 44, 48, 52, 56, 60});
}

/// Pattern 8: (ABCD)x6 (ABC) (ABCD)x12
TEST_F(EditDistEpochRecordFilterTest, pattern_subtract1)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "8_pattern_subtract1.trace", history_size);
    check_vals(testout, {11, 15, 19, 23, 46, 50, 54, 58, 62, 66, 70, 74});
}

/// FFT Short for Rank 0
TEST_F(EditDistEpochRecordFilterTest, fft_small)
{
    int history_size = 20;

    std::vector<record_s> testout = filter_file(m_trace_file_prefix + "fft_small.trace", history_size);
    check_vals(testout, {11, 13, 16, 19, 21, 24, 27});
}

TEST_F(EditDistEpochRecordFilterTest, parse_name)
{
    int buffer_size = -1;
    double stable_hyst = NAN;
    int min_hysteresis_base_period = -1;
    int min_detectable_period = -1;
    double unstable_hyst = NAN;

    EditDistEpochRecordFilter::parse_name("edit_distance",
                                          buffer_size,
                                          min_hysteresis_base_period,
                                          min_detectable_period,
                                          stable_hyst,
                                          unstable_hyst);
    // default values
    EXPECT_EQ(50, buffer_size);
    EXPECT_EQ(1.0, stable_hyst);
    EXPECT_EQ(4, min_hysteresis_base_period);
    EXPECT_EQ(3, min_detectable_period);
    EXPECT_EQ(1.5, unstable_hyst);

    EditDistEpochRecordFilter::parse_name("edit_distance,42",
                                          buffer_size,
                                          min_hysteresis_base_period,
                                          min_detectable_period,
                                          stable_hyst,
                                          unstable_hyst);
    EXPECT_EQ(42, buffer_size);
    EXPECT_EQ(4, min_hysteresis_base_period);
    EXPECT_EQ(3, min_detectable_period);
    EXPECT_EQ(1.0, stable_hyst);
    EXPECT_EQ(1.5, unstable_hyst);

    EditDistEpochRecordFilter::parse_name("edit_distance,52,20",
                                          buffer_size,
                                          min_hysteresis_base_period,
                                          min_detectable_period,
                                          stable_hyst,
                                          unstable_hyst);
    EXPECT_EQ(52, buffer_size);
    EXPECT_EQ(20, min_hysteresis_base_period);
    EXPECT_EQ(3, min_detectable_period);
    EXPECT_EQ(1.0, stable_hyst);
    EXPECT_EQ(1.5, unstable_hyst);

    EditDistEpochRecordFilter::parse_name("edit_distance,52,20,105",
                                          buffer_size,
                                          min_hysteresis_base_period,
                                          min_detectable_period,
                                          stable_hyst,
                                          unstable_hyst);
    EXPECT_EQ(52, buffer_size);
    EXPECT_EQ(20, min_hysteresis_base_period);
    EXPECT_EQ(105, min_detectable_period);
    EXPECT_EQ(1.0, stable_hyst);
    EXPECT_EQ(1.5, unstable_hyst);

    EditDistEpochRecordFilter::parse_name("edit_distance,62,30,115,5.0",
                                          buffer_size,
                                          min_hysteresis_base_period,
                                          min_detectable_period,
                                          stable_hyst,
                                          unstable_hyst);
    EXPECT_EQ(62, buffer_size);
    EXPECT_EQ(30, min_hysteresis_base_period);
    EXPECT_EQ(115, min_detectable_period);
    EXPECT_EQ(5.0, stable_hyst);
    EXPECT_EQ(1.5, unstable_hyst);

    EditDistEpochRecordFilter::parse_name("edit_distance,62,40,125,6.0,3.5",
                                          buffer_size,
                                          min_hysteresis_base_period,
                                          min_detectable_period,
                                          stable_hyst,
                                          unstable_hyst);
    EXPECT_EQ(62, buffer_size);
    EXPECT_EQ(40, min_hysteresis_base_period);
    EXPECT_EQ(125, min_detectable_period);
    EXPECT_EQ(6.0, stable_hyst);
    EXPECT_EQ(3.5, unstable_hyst);

    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("not_edit_distance",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "Unknown filter name");
    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("edit_distance,invalid",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "invalid buffer size");
    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("edit_distance,1,invalid",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "invalid hysteresis base period");
    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("edit_distance,1,1,invalid",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "invalid minimum detectable period");
    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("edit_distance,1,1,1,invalid",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "invalid stable hysteresis");
    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("edit_distance,1,1,1,1,invalid",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "invalid unstable hysteresis");
    GEOPM_EXPECT_THROW_MESSAGE(EditDistEpochRecordFilter::parse_name("edit_distance,1,1,1,2,2,2",
                                                                     buffer_size,
                                                                     min_hysteresis_base_period,
                                                                     min_detectable_period,
                                                                     stable_hyst,
                                                                     unstable_hyst),
                               GEOPM_ERROR_INVALID, "Too many commas");
}

std::vector<record_s> EditDistEpochRecordFilterTest::filter_file(std::string trace_file_path,
                                                                 int buffer_size, int min_detectable_period)
{
    MockApplicationSampler app;
    app.inject_records(geopm::read_file(trace_file_path));

    geopm::EditDistEpochRecordFilter ederf(buffer_size,
                                           m_min_hysteresis_base_period,
                                           min_detectable_period,
                                           m_stable_hyst,
                                           m_unstable_hyst);

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

std::vector<record_s> EditDistEpochRecordFilterTest::filter_file(std::string trace_file_path,
                                                                 int buffer_size)
{
    return filter_file(trace_file_path, buffer_size, 1);
}
/// HELPER FUNCTIONS

void check_vals(std::vector<record_s> testout, std::vector<int> epoch_time_vector)
{
    EXPECT_EQ(epoch_time_vector, extract_epoch_times(testout));
}

std::vector<int> extract_epoch_times(std::vector<record_s> recs)
{
    std::vector<int> results;
    for(const auto &rec : recs) {
        if(rec.event == geopm::EVENT_EPOCH_COUNT) {
            results.push_back(rec.time.t.tv_sec);
        }
    }
    return results;
}
