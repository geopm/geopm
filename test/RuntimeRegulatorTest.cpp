/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#define GEOPM_TEST
#include "geopm_time.h"
#include "geopm_message.h"
#include "Exception.hpp"
#include "RuntimeRegulator.hpp"

using geopm::Exception;
using geopm::RuntimeRegulator;

class RuntimeRegulatorTest : public :: testing :: Test
{
    public:
        static const int M_NUM_RANKS = 4;
        static const int M_NUM_ITERATIONS = 4;
        static constexpr int M_RANK_TIMES[M_NUM_RANKS][M_NUM_ITERATIONS] =
                            {
                                {2, 4, 6, 8},
                                {8, 9, 10, 11},
                                {0, 16, 32, 64},
                                {10, 20, 30, 40}
                            };
    protected:
        void SetUp();
        void TearDown();
        void validate(double expected_avg);

        static constexpr double M_ERROR_BOUNDS = 1e-3;// TODO better error range val?
        struct geopm_time_s m_entry[M_NUM_RANKS][M_NUM_ITERATIONS];
        struct geopm_time_s m_exit[M_NUM_RANKS][M_NUM_ITERATIONS];
        std::vector<struct geopm_telemetry_message_s> m_telemetry_sample;
};

constexpr double RuntimeRegulatorTest::M_ERROR_BOUNDS;
constexpr int RuntimeRegulatorTest::M_RANK_TIMES[RuntimeRegulatorTest::M_NUM_RANKS][RuntimeRegulatorTest::M_NUM_ITERATIONS];

void RuntimeRegulatorTest::SetUp()
{
    m_telemetry_sample.resize(1, {0, {{0, 0}}, {0}});

    for (int x = 0; x < M_NUM_RANKS; x++) {
        for (int y = 0; y < M_NUM_ITERATIONS; y++) {
            m_entry[x][y] = {{0, 0}};
            m_exit[x][y] = (struct geopm_time_s) {{(time_t) M_RANK_TIMES[x][y], 0}};
        }
    }
}

void RuntimeRegulatorTest::TearDown()
{
}

void RuntimeRegulatorTest::validate(double expected_avg)
{
    EXPECT_NEAR(expected_avg, m_telemetry_sample[0].signal[GEOPM_TELEMETRY_TYPE_RUNTIME],
                M_ERROR_BOUNDS);
    for (int x = 0; x < GEOPM_NUM_TELEMETRY_TYPE; ++x) {
        if (x != GEOPM_TELEMETRY_TYPE_RUNTIME) {
            EXPECT_EQ(0, m_telemetry_sample[0].signal[x]);
        }
    }
}

TEST_F(RuntimeRegulatorTest, exceptions)
{
    EXPECT_THROW(new RuntimeRegulator(0), Exception);
    RuntimeRegulator rtr(M_NUM_RANKS);
    EXPECT_THROW(rtr.record_entry(-1, m_entry[0][0]), Exception);
    EXPECT_THROW(rtr.record_exit(-1, m_exit[0][0]), Exception);
}

TEST_F(RuntimeRegulatorTest, all_in_and_out)
{
    static constexpr double expected_avg[M_NUM_ITERATIONS] =
    {
        6.666,
        12.25,
        19.5,
        30.75
    };

    for (int y = 0; y < M_NUM_ITERATIONS; y++) {
        RuntimeRegulator rtr(M_NUM_RANKS);
        int x;
        for (x = 0; x < M_NUM_RANKS; x++) {
            if (M_RANK_TIMES[x][y]) {
                rtr.record_entry(x, m_entry[x][y]);
            }
        }
        for (x = 0; x < M_NUM_RANKS; x++) {
            if (M_RANK_TIMES[x][y]) {
                rtr.record_exit(x, m_exit[x][y]);
            }
        }

        rtr.insert_runtime_signal(m_telemetry_sample);
        validate(expected_avg[y]);
    }
}

TEST_F(RuntimeRegulatorTest, all_reenter)
{
    static const double expected_avg = 12.25;

    int y = 1;
    RuntimeRegulator rtr(M_NUM_RANKS);
    int x;
    for (x = 0; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_entry(x, m_entry[x][y]);
        }
    }
    for (x = 0; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_exit(x, m_exit[x][y]);
        }
    }
    y = 2;
    for (x = 0; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_entry(x, m_entry[x][y]);
        }
    }
    rtr.insert_runtime_signal(m_telemetry_sample);
    validate(expected_avg);
}

TEST_F(RuntimeRegulatorTest, one_rank_reenter_and_exit)
{
    int y = 1;
    RuntimeRegulator rtr(M_NUM_RANKS);
    int x;
    for (x = 0; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_entry(x, m_entry[x][y]);
        }
    }
    for (x = 0; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_exit(x, m_exit[x][y]);
        }
    }
    y = 2;
    x = 0;
    rtr.record_entry(x, m_entry[x][y]);
    rtr.record_exit(x, m_exit[x][y]);

    rtr.insert_runtime_signal(m_telemetry_sample);
    validate(M_RANK_TIMES[x][y]);
}

TEST_F(RuntimeRegulatorTest, config_rank_then_workers)
{
    static const double expected_avg = 15.0;
    int y = 1;
    RuntimeRegulator rtr(M_NUM_RANKS);
    int x = 0;

    rtr.record_entry(x, m_entry[x][y]);
    rtr.record_exit(x, m_exit[x][y]);

    for (x = 1; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_entry(x, m_entry[x][y]);
        }
    }
    for (x = 1; x < M_NUM_RANKS; x++) {
        if (M_RANK_TIMES[x][y]) {
            rtr.record_exit(x, m_exit[x][y]);
        }
    }
    y = 2;

    rtr.insert_runtime_signal(m_telemetry_sample);
    validate(expected_avg);
}
