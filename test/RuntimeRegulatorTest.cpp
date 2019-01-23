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
#include "geopm_time.h"
#include "Exception.hpp"
#include "RuntimeRegulator.hpp"

using geopm::Exception;
using geopm::RuntimeRegulator;

class RuntimeRegulatorTest : public :: testing :: Test
{
    public:
        static const int M_NUM_RANKS = 4;
        static const int M_NUM_ITERATIONS = 4;
        static constexpr int M_RANK_TIMES[M_NUM_ITERATIONS][M_NUM_RANKS] =
        {
            {2, 8, 0, 10},
            {4, 9, 16, 20},
            {6, 10, 32, 30},
            {8, 11, 64, 40}
        };
    protected:
        void SetUp();
        void TearDown();

        struct geopm_time_s m_entry[M_NUM_ITERATIONS][M_NUM_RANKS];
        struct geopm_time_s m_exit[M_NUM_ITERATIONS][M_NUM_RANKS];
        std::vector<double> m_total_runtime;
};

constexpr int RuntimeRegulatorTest::M_RANK_TIMES[RuntimeRegulatorTest::M_NUM_RANKS][RuntimeRegulatorTest::M_NUM_ITERATIONS];

void RuntimeRegulatorTest::SetUp()
{
    m_total_runtime.resize(M_NUM_RANKS);
    for (int it = 0; it < M_NUM_ITERATIONS; ++it) {
        for (int rank = 0; rank < M_NUM_RANKS; ++rank) {
            m_entry[it][rank] = {{1, 0}};  // must be non-zero because zero marks an exit
            m_exit[it][rank] = (struct geopm_time_s) {{(time_t) M_RANK_TIMES[it][rank] + 1, 0}};
            m_total_runtime[rank] += M_RANK_TIMES[it][rank];
        }
    }
}

void RuntimeRegulatorTest::TearDown()
{
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
    RuntimeRegulator rtr(M_NUM_RANKS);
    std::vector<double> expected(M_NUM_RANKS);
    for (int it = 0; it < M_NUM_ITERATIONS; it++) {
        for (int rank = 0; rank < M_NUM_RANKS; rank++) {
            rtr.record_entry(rank, m_entry[it][rank]);
        }
        for (int rank = 0; rank < M_NUM_RANKS; rank++) {
            rtr.record_exit(rank, m_exit[it][rank]);
            expected[rank] = M_RANK_TIMES[it][rank];
        }
        auto result = rtr.per_rank_last_runtime();
        EXPECT_EQ(expected, result);
    }
    auto result = rtr.per_rank_total_runtime();
    EXPECT_EQ(m_total_runtime, result);
    std::vector<double> exp_count(M_NUM_RANKS, M_NUM_ITERATIONS);
    EXPECT_EQ(exp_count, rtr.per_rank_count());
}

TEST_F(RuntimeRegulatorTest, all_reenter)
{
    RuntimeRegulator rtr(M_NUM_RANKS);
    std::vector<double> expected(M_NUM_RANKS);
    int it = 1;
    for (int rank = 0; rank < M_NUM_RANKS; ++rank) {
        rtr.record_entry(rank, m_entry[it][rank]);
    }
    for (int rank = 0; rank < M_NUM_RANKS; ++rank) {
        rtr.record_exit(rank, m_exit[it][rank]);
        expected[rank] = M_RANK_TIMES[it][rank];
    }
    it = 2;
    for (int rank = 0; rank < M_NUM_RANKS; ++rank) {
        rtr.record_entry(rank, m_entry[it][rank]);
    }
    EXPECT_EQ(expected, rtr.per_rank_last_runtime());
    EXPECT_EQ(expected, rtr.per_rank_total_runtime());
    std::vector<double> exp_count(M_NUM_RANKS, 1);
    EXPECT_EQ(exp_count, rtr.per_rank_count());
}

TEST_F(RuntimeRegulatorTest, one_rank_reenter_and_exit)
{
    RuntimeRegulator rtr(M_NUM_RANKS);
    int it = 1;
    for (int rank = 0; rank < M_NUM_RANKS; rank++) {
        rtr.record_entry(rank, m_entry[it][rank]);
    }
    for (int rank = 0; rank < M_NUM_RANKS; rank++) {
        rtr.record_exit(rank, m_exit[it][rank]);
    }
    it = 2;
    int rank = 0;
    rtr.record_entry(rank, m_entry[it][rank]);
    rtr.record_exit(rank, m_exit[it][rank]);
    EXPECT_EQ(M_RANK_TIMES[it][rank], rtr.per_rank_last_runtime()[rank]);
    double exp_rank0 = M_RANK_TIMES[1][rank] + M_RANK_TIMES[2][rank];
    EXPECT_EQ(exp_rank0, rtr.per_rank_total_runtime()[rank]);
    EXPECT_EQ(2, rtr.per_rank_count()[rank]);
    // other ranks still have old runtime and count
    rank = 1;
    EXPECT_EQ(M_RANK_TIMES[1][rank], rtr.per_rank_last_runtime()[rank]);
    EXPECT_EQ(M_RANK_TIMES[1][rank], rtr.per_rank_total_runtime()[rank]);
    EXPECT_EQ(1, rtr.per_rank_count()[rank]);
}

TEST_F(RuntimeRegulatorTest, config_rank_then_workers)
{
    RuntimeRegulator rtr(M_NUM_RANKS);
    std::vector<double> expected(M_NUM_RANKS);
    int it = 1;
    for (int rr = 0; rr < M_NUM_RANKS; ++rr) {
        expected[rr] = M_RANK_TIMES[it][rr];
    }

    int rank = 0;
    rtr.record_entry(rank, m_entry[it][rank]);
    rtr.record_exit(rank, m_exit[it][rank]);

    for (rank = 1; rank < M_NUM_RANKS; rank++) {
        rtr.record_entry(rank, m_entry[it][rank]);
    }
    for (rank = 1; rank < M_NUM_RANKS; rank++) {
        rtr.record_exit(rank, m_exit[it][rank]);
    }
    EXPECT_EQ(expected, rtr.per_rank_last_runtime());
    EXPECT_EQ(expected, rtr.per_rank_total_runtime());
    std::vector<double> exp_count(M_NUM_RANKS, 1);
    EXPECT_EQ(exp_count, rtr.per_rank_count());
}
