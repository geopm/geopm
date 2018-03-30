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
#include "gmock/gmock.h"

#include "ProfileIORuntime.hpp"
#include "MockRuntimeRegulator.hpp"
#include "geopm_test.hpp"

using testing::Return;
using testing::_;
using geopm::ProfileIORuntime;

TEST(ProfileIORuntimeTest, per_cpu_runtime)
{
    MockRuntimeRegulator mock_reg_1;
    MockRuntimeRegulator mock_reg_2;
    std::vector<int> cpu_rank {1, 1, 2, 2, 3, 3, 4, 4};
    ProfileIORuntime m_profile_runtime(cpu_rank);
    uint64_t region_id_1 = 999;
    uint64_t region_id_2 = 777;
    m_profile_runtime.insert_regulator(region_id_1, mock_reg_1);
    m_profile_runtime.insert_regulator(region_id_2, mock_reg_2);

    std::vector<double> rank_runtime_1{8, 6, 8, 5};
    std::vector<double> rank_runtime_2{9, 7, 5, 4};
    std::vector<double> expected_runtime_1{8, 8, 6, 6, 8, 8, 5, 5};
    std::vector<double> expected_runtime_2{9, 9, 7, 7, 5, 5, 4, 4};

    EXPECT_CALL(mock_reg_1, runtimes())
        .WillOnce(Return(rank_runtime_1));
    EXPECT_CALL(mock_reg_2, runtimes())
        .WillOnce(Return(rank_runtime_2));

    std::vector<double> runtime = m_profile_runtime.per_cpu_runtime(region_id_1);
    EXPECT_EQ(expected_runtime_1, runtime);
    runtime = m_profile_runtime.per_cpu_runtime(region_id_2);
    EXPECT_EQ(expected_runtime_2, runtime);

    // errors
#ifdef GEOPM_DEBUG
    GEOPM_EXPECT_THROW_MESSAGE(m_profile_runtime.per_cpu_runtime(808080),
                               GEOPM_ERROR_LOGIC, "No regulator set for region");
#endif
}

TEST(ProfileIORuntimeTest, per_rank_runtime)
{
    MockRuntimeRegulator mock_reg_1;
    MockRuntimeRegulator mock_reg_2;
    std::vector<int> cpu_rank {1, 1, 2, 2, 3, 3, 4, 4};
    ProfileIORuntime m_profile_runtime(cpu_rank);
    uint64_t region_id_1 = 999;
    uint64_t region_id_2 = 777;
    m_profile_runtime.insert_regulator(region_id_1, mock_reg_1);
    m_profile_runtime.insert_regulator(region_id_2, mock_reg_2);

    std::vector<double> rank_runtime_1{8, 6, 8, 5};
    std::vector<double> rank_runtime_2{9, 7, 5, 4};

    EXPECT_CALL(mock_reg_1, runtimes())
        .WillOnce(Return(rank_runtime_1));
    EXPECT_CALL(mock_reg_2, runtimes())
        .WillOnce(Return(rank_runtime_2));

    std::vector<double> runtime = m_profile_runtime.per_rank_runtime(region_id_1);
    EXPECT_EQ(rank_runtime_1, runtime);
    runtime = m_profile_runtime.per_rank_runtime(region_id_2);
    EXPECT_EQ(rank_runtime_2, runtime);

    // errors
#ifdef GEOPM_DEBUG
    GEOPM_EXPECT_THROW_MESSAGE(m_profile_runtime.per_rank_runtime(808080),
                               GEOPM_ERROR_LOGIC, "No regulator set for region");
#endif
}
