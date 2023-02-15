/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <cstdint>

#include "gtest/gtest.h"

#include "EditDistPeriodicityDetector.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

using geopm::record_s;
using geopm::EditDistPeriodicityDetector;

class EditDistPeriodicityDetectorTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::string m_trace_file_prefix;
};


void EditDistPeriodicityDetectorTest::SetUp()
{
    m_trace_file_prefix = GEOPM_SOURCE_DIR;
    m_trace_file_prefix += "/test/EditDistPeriodicityDetectorTest.";
}

void check_vals(std::string trace_file_path, int warmup, int period, int history_size=20);
void check_vals(std::string trace_file_path, int start, int end, int period, int history_size);
void check_vals(std::vector<record_s> recs, std::vector<std::vector<int> > expected, int history_size=20);


/// Pattern 0: (A)x10
TEST_F(EditDistPeriodicityDetectorTest, pattern_a)
{
    int warmup = 1;
    int period = 1;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "0_pattern_a.trace", warmup, period, history_size);
}

/// Pattern 1: (AB)x15
TEST_F(EditDistPeriodicityDetectorTest, pattern_ab)
{
    int warmup = 3;
    int period = 2;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "1_pattern_ab.trace", warmup, period, history_size);
}

/// Pattern 2: (ABB)x12
TEST_F(EditDistPeriodicityDetectorTest, pattern_abb)
{
    int warmup = 5;
    int period = 3;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "2_pattern_abb.trace", warmup, period, history_size);
}

/// Pattern 3: (ABCDABCDABCDC) (ABCDABCDABCDABCDC)x6 (ABCD)
TEST_F(EditDistPeriodicityDetectorTest, pattern_abcdc)
{
    int warmup = 33;
    int period = 17;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "3_pattern_abcdc.trace", warmup, period, history_size);
}

/// Pattern 4: (AB) (ABABC)x3
TEST_F(EditDistPeriodicityDetectorTest, pattern_ababc)
{
    int warmup = 11;
    int period = 5;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "4_pattern_ababc.trace", warmup, period, history_size);
}

/// Pattern 5: (ABABABC)x6
TEST_F(EditDistPeriodicityDetectorTest, pattern_abababc)
{
    int warmup = 13;
    int period = 7;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "5_pattern_abababc.trace", warmup, period, history_size);
}

/// Pattern 6: (ABCD)x6 (E) (ABCD)x6
TEST_F(EditDistPeriodicityDetectorTest, pattern_add1)
{
    int period = 4;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "6_pattern_add1.trace", 7, 24, period, history_size);
    int warmup = 32;
    check_vals(m_trace_file_prefix + "6_pattern_add1.trace", warmup, period, history_size);
}

/// Pattern 7: (ABCD)x6 (EF) (ABCD)x9
TEST_F(EditDistPeriodicityDetectorTest, pattern_add2)
{
    int period = 4;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "7_pattern_add2.trace", 7, 24, period, history_size);
    check_vals(m_trace_file_prefix + "7_pattern_add2.trace", 33, period, history_size);
}

/// Pattern 8: (ABCD)x6 (ABC) (ABCD)x12
TEST_F(EditDistPeriodicityDetectorTest, pattern_subtract1)
{
    int period = 4;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "8_pattern_subtract1.trace", 7, 27, period, history_size);
    check_vals(m_trace_file_prefix + "8_pattern_subtract1.trace", 54, period, history_size);
}

/// FFT Short for Rank 0
TEST_F(EditDistPeriodicityDetectorTest, fft_small)
{
    int warmup = 60;
    int period = 13;
    int history_size = 20;

    check_vals(m_trace_file_prefix + "fft_small.trace", warmup, period, history_size);
}

/// HELPER FUNCTIONS

/// start: inclusive
/// end: exclusive
void check_vals(std::string trace_file_path, int start, int end, int period, int history_size)
{
    MockApplicationSampler app;
    app.inject_records(geopm::read_file(trace_file_path));

    std::vector<std::vector<int> > expected;

    for(int i = start; i > 0; i--) {
        expected.push_back({-1, 0});
    }

    std::vector<record_s> recs = app.get_records();

    int valid_recs = recs.size()-start;
    if(end-start < valid_recs) {
        valid_recs = end-start;
    }

    for(int i = valid_recs; i > 0; i--) {
        expected.push_back({period, 0});
    }

    check_vals(recs, expected, history_size);
}

void check_vals(std::string trace_file_path, int warmup, int period, int history_size)
{
    MockApplicationSampler app;
    app.inject_records(geopm::read_file(trace_file_path));

    std::vector<std::vector<int> > expected;

    for(int i = warmup; i > 0; i--) {
        expected.push_back({-1, 0});
    }

    std::vector<record_s> recs = app.get_records();

    int valid_recs = recs.size()-warmup;

    for(int i = valid_recs; i > 0; i--) {
        expected.push_back({period, 0});
    }

    check_vals(recs, expected, history_size);
}

void check_vals(std::vector<record_s> recs, std::vector<std::vector<int> > expected, int history_size)
{
    geopm::EditDistPeriodicityDetector edpd(history_size);

    int region_entry_num = 0;
    for(size_t i = 0; i < expected.size(); i++) {
        if (recs[i].event == geopm::EVENT_REGION_ENTRY) {
            edpd.update(recs[i]);
            int period = edpd.get_period();
            int score = edpd.get_score();

            if(expected[region_entry_num][0] != -1) {
                // -1 means skip the test entry (probably for warmup).
                EXPECT_EQ(expected[region_entry_num][0], edpd.get_period()) << "Record #: " << region_entry_num << " Received period=" << period << " Expected period=" <<  expected[region_entry_num][0] << "\n";
                EXPECT_EQ(expected[region_entry_num][1], edpd.get_score()) << "Record #: " << region_entry_num << " Received score=" << score << " Expected score=" << expected[region_entry_num][1] << "\n";
            }
            region_entry_num++;
        }
    }
}

