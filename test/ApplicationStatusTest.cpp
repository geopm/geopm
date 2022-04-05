/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#include "geopm_test.hpp"

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm_hash.h"
#include "ApplicationStatus.hpp"
#include "MockSharedMemory.hpp"

using geopm::ApplicationStatus;
using testing::AtLeast;
using testing::Return;

class ApplicationStatusTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockSharedMemory> m_mock_shared_memory;
        std::unique_ptr<ApplicationStatus> m_status;
        const int M_NUM_CPU = 4;
};

void ApplicationStatusTest::SetUp()
{
    size_t buffer_size = ApplicationStatus::buffer_size(M_NUM_CPU);
    m_mock_shared_memory = std::make_shared<MockSharedMemory>(buffer_size);
    m_status = ApplicationStatus::make_unique(M_NUM_CPU, m_mock_shared_memory);
}

TEST_F(ApplicationStatusTest, wrong_buffer_size)
{
    auto shmem = std::make_shared<MockSharedMemory>(7);
    GEOPM_EXPECT_THROW_MESSAGE(ApplicationStatus::make_unique(M_NUM_CPU, shmem),
                               GEOPM_ERROR_INVALID, "shared memory incorrectly sized");
}

TEST_F(ApplicationStatusTest, bad_shmem)
{
    GEOPM_EXPECT_THROW_MESSAGE(ApplicationStatus::make_unique(M_NUM_CPU, nullptr),
                               GEOPM_ERROR_INVALID, "shared memory pointer cannot be null");
}

TEST_F(ApplicationStatusTest, hints)
{
    uint64_t NOHINTS = GEOPM_REGION_HINT_UNSET;
    uint64_t NETWORK = GEOPM_REGION_HINT_NETWORK;
    uint64_t COMPUTE = GEOPM_REGION_HINT_COMPUTE;
    uint64_t INACTIVE = GEOPM_REGION_HINT_INACTIVE;

    EXPECT_EQ(INACTIVE, m_status->get_hint(0));
    EXPECT_EQ(INACTIVE, m_status->get_hint(1));
    EXPECT_EQ(INACTIVE, m_status->get_hint(2));
    EXPECT_EQ(INACTIVE, m_status->get_hint(3));

    m_status->set_process({0, 1, 2, 3}, 123);
    m_status->update_cache();
    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NOHINTS, m_status->get_hint(1));
    EXPECT_EQ(NOHINTS, m_status->get_hint(2));
    EXPECT_EQ(NOHINTS, m_status->get_hint(3));

    m_status->set_hint(1, NETWORK);
    m_status->set_hint(3, NETWORK);
    m_status->update_cache();
    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NETWORK, m_status->get_hint(1));
    EXPECT_EQ(NOHINTS, m_status->get_hint(2));
    EXPECT_EQ(NETWORK, m_status->get_hint(3));

    m_status->set_hint(2, COMPUTE);
    m_status->set_hint(3, COMPUTE);
    m_status->update_cache();
    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NETWORK, m_status->get_hint(1));
    EXPECT_EQ(COMPUTE, m_status->get_hint(2));
    EXPECT_EQ(COMPUTE, m_status->get_hint(3));

    // clear hint
    m_status->set_hint(1, GEOPM_REGION_HINT_UNSET);
    m_status->set_hint(2, GEOPM_REGION_HINT_UNSET);
    m_status->set_hint(3, GEOPM_REGION_HINT_UNSET);
    m_status->update_cache();
    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NOHINTS, m_status->get_hint(1));
    EXPECT_EQ(NOHINTS, m_status->get_hint(2));
    EXPECT_EQ(NOHINTS, m_status->get_hint(3));


    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hint(-1, NETWORK),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hint(99, NETWORK),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hint(0, 2),
                               GEOPM_ERROR_INVALID, "invalid hint");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hint(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hint(99),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    std::vector<uint64_t> bad_data(8, ~0ULL);
    memcpy(m_mock_shared_memory->pointer(), bad_data.data(), 64);
    m_status->update_cache();
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hint(0),
                               GEOPM_ERROR_INVALID, "invalid hint");
}

TEST_F(ApplicationStatusTest, hash)
{
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(0));
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(1));
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(2));
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(3));

    m_status->set_hash(0, 0xAA, GEOPM_REGION_HINT_MEMORY);
    m_status->set_hash(1, 0xAA, GEOPM_REGION_HINT_NETWORK);
    m_status->set_hash(2, 0xBB, GEOPM_REGION_HINT_COMPUTE);
    m_status->set_hash(3, 0xCC, GEOPM_REGION_HINT_IGNORE);
    m_status->update_cache();
    EXPECT_EQ(0xAAULL, m_status->get_hash(0));
    EXPECT_EQ(0xAAULL, m_status->get_hash(1));
    EXPECT_EQ(0xBBULL, m_status->get_hash(2));
    EXPECT_EQ(0xCCULL, m_status->get_hash(3));
    EXPECT_EQ(GEOPM_REGION_HINT_MEMORY, m_status->get_hint(0));
    EXPECT_EQ(GEOPM_REGION_HINT_NETWORK, m_status->get_hint(1));
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, m_status->get_hint(2));
    EXPECT_EQ(GEOPM_REGION_HINT_IGNORE, m_status->get_hint(3));

    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hash(-1, 0xDD, GEOPM_REGION_HINT_UNSET),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hash(99, 0xDD, GEOPM_REGION_HINT_UNSET),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hash(0, (0xFFULL << 32), GEOPM_REGION_HINT_UNSET),
                               GEOPM_ERROR_INVALID, "invalid region hash");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hash(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hash(99),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
}

TEST_F(ApplicationStatusTest, work_progress)
{
    // CPUs 2 and 3 are inactive, 0 work units
    m_status->reset_work_units(0);
    m_status->set_total_work_units(0, 4);
    m_status->reset_work_units(1);
    m_status->set_total_work_units(1, 8);
    m_status->update_cache();
    EXPECT_DOUBLE_EQ(0.000, m_status->get_progress_cpu(0));
    EXPECT_DOUBLE_EQ(0.000, m_status->get_progress_cpu(1));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(2)));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(3)));
    m_status->increment_work_unit(0);
    m_status->increment_work_unit(1);
    m_status->update_cache();
    EXPECT_DOUBLE_EQ(0.250, m_status->get_progress_cpu(0));
    EXPECT_DOUBLE_EQ(0.125, m_status->get_progress_cpu(1));
    m_status->increment_work_unit(0);
    m_status->update_cache();
    EXPECT_DOUBLE_EQ(0.500, m_status->get_progress_cpu(0));
    EXPECT_DOUBLE_EQ(0.125, m_status->get_progress_cpu(1));
    EXPECT_DOUBLE_EQ(0.125, m_status->get_progress_cpu(1));
    m_status->increment_work_unit(0);
    m_status->increment_work_unit(1);
    m_status->update_cache();
    EXPECT_DOUBLE_EQ(0.750, m_status->get_progress_cpu(0));
    EXPECT_DOUBLE_EQ(0.250, m_status->get_progress_cpu(1));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(2)));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(3)));
    m_status->increment_work_unit(0);
    m_status->increment_work_unit(1);
    m_status->increment_work_unit(1);
    m_status->update_cache();
    EXPECT_DOUBLE_EQ(1.000, m_status->get_progress_cpu(0));
    EXPECT_DOUBLE_EQ(0.500, m_status->get_progress_cpu(1));

    // reset progress
    m_status->reset_work_units(0);
    m_status->set_total_work_units(0, 1);
    m_status->update_cache();
    EXPECT_DOUBLE_EQ(0.00, m_status->get_progress_cpu(0));

    // leave region
    m_status->reset_work_units(0);
    m_status->reset_work_units(1);
    m_status->reset_work_units(2);
    m_status->reset_work_units(3);
    m_status->update_cache();
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(0)));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(1)));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(2)));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(3)));

    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_progress_cpu(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_progress_cpu(99),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->reset_work_units(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_total_work_units(-1, 100),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_total_work_units(99, 100),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->increment_work_unit(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->increment_work_unit(99),
                               GEOPM_ERROR_INVALID, "invalid CPU index");

    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_total_work_units(0, -10),
                               GEOPM_ERROR_INVALID, "invalid number of work units");
}

TEST_F(ApplicationStatusTest, process)
{
    EXPECT_EQ(-1, m_status->get_process(0));
    EXPECT_EQ(-1, m_status->get_process(1));
    EXPECT_EQ(-1, m_status->get_process(2));
    EXPECT_EQ(-1, m_status->get_process(3));

    m_status->set_process({0, 2}, 34);
    m_status->set_process({1}, 56);
    m_status->set_process({3}, 78);
    m_status->update_cache();
    EXPECT_EQ(34, m_status->get_process(0));
    EXPECT_EQ(56, m_status->get_process(1));
    EXPECT_EQ(34, m_status->get_process(2));
    EXPECT_EQ(78, m_status->get_process(3));

    // detach processes
    m_status->set_process({0, 1, 2, 3}, -1);
    m_status->update_cache();
    EXPECT_EQ(-1, m_status->get_process(0));
    EXPECT_EQ(-1, m_status->get_process(1));
    EXPECT_EQ(-1, m_status->get_process(2));
    EXPECT_EQ(-1, m_status->get_process(3));

    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_process({-1}, 2),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_process({99}, 2),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_process(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_process(99),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
}

TEST_F(ApplicationStatusTest, update_cache)
{
    uint64_t hint = GEOPM_REGION_HINT_NETWORK;
    uint64_t hash = 0xABC;
    int process = 42;
    EXPECT_EQ(GEOPM_REGION_HINT_INACTIVE, m_status->get_hint(0));
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(0));
    m_status->set_process({0, 1}, process);

    // default values before cache update
    EXPECT_EQ(-1, m_status->get_process(0));

    m_status->update_cache();
    // set process initalizes hash for active CPUs
    EXPECT_EQ(GEOPM_REGION_HASH_UNMARKED, m_status->get_hash(0));
    EXPECT_EQ(GEOPM_REGION_HASH_UNMARKED, m_status->get_hash(1));
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(2));
    EXPECT_EQ(GEOPM_REGION_HASH_INVALID, m_status->get_hash(3));

    m_status->set_hash(0, hash, hint);
    m_status->set_total_work_units(0, 4);
    m_status->increment_work_unit(0);
    // default values before cache update
    EXPECT_EQ(GEOPM_REGION_HINT_UNSET, m_status->get_hint(0));
    EXPECT_EQ(GEOPM_REGION_HASH_UNMARKED, m_status->get_hash(0));
    EXPECT_TRUE(std::isnan(m_status->get_progress_cpu(0)));

    // written values visible after update
    m_status->update_cache();
    EXPECT_EQ(hint, m_status->get_hint(0));
    EXPECT_EQ(hash, m_status->get_hash(0));
    EXPECT_EQ(0.25, m_status->get_progress_cpu(0));
    EXPECT_EQ(process, m_status->get_process(0));

    m_status->set_hash(0, GEOPM_REGION_HASH_INVALID, GEOPM_REGION_HINT_UNSET);
    m_status->set_total_work_units(0, 8);
    m_status->increment_work_unit(0);
    m_status->set_process({0, 1}, process);

    // same values until next update
    EXPECT_EQ(hint, m_status->get_hint(0));
    EXPECT_EQ(hash, m_status->get_hash(0));
    EXPECT_EQ(0.25, m_status->get_progress_cpu(0));
    EXPECT_EQ(process, m_status->get_process(0));

}
