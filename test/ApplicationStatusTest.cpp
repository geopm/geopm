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

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"

#include "geopm.h"
#include "ApplicationStatus.hpp"
#include "MockSharedMemory.hpp"
#include "MockPlatformTopo.hpp"

using geopm::ApplicationStatus;
using testing::AtLeast;
using testing::Return;

class ApplicationStatusTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockSharedMemory> m_mock_shared_memory;
        std::unique_ptr<ApplicationStatus> m_status;
        MockPlatformTopo m_topo;
        const int M_NUM_CPU = 4;
};

void ApplicationStatusTest::SetUp()
{
    ON_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillByDefault(Return(M_NUM_CPU));

    size_t buffer_size = ApplicationStatus::buffer_size(M_NUM_CPU);
    m_mock_shared_memory = std::make_shared<MockSharedMemory>(buffer_size);

    EXPECT_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU));
    m_status = ApplicationStatus::make_unique(m_topo, m_mock_shared_memory);


}

TEST_F(ApplicationStatusTest, wrong_buffer_size)
{
    auto shmem = std::make_shared<MockSharedMemory>(7);
    EXPECT_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU));
    GEOPM_EXPECT_THROW_MESSAGE(ApplicationStatus::make_unique(m_topo, shmem),
                               GEOPM_ERROR_INVALID, "shared memory incorrectly sized");
}

TEST_F(ApplicationStatusTest, bad_shmem)
{
    EXPECT_CALL(m_topo, num_domain(GEOPM_DOMAIN_CPU));
    GEOPM_EXPECT_THROW_MESSAGE(ApplicationStatus::make_unique(m_topo, nullptr),
                               GEOPM_ERROR_INVALID, "shared memory pointer cannot be null");
}

TEST_F(ApplicationStatusTest, hints)
{
    uint64_t NOHINTS = 0ULL;
    uint64_t NETWORK = GEOPM_REGION_HINT_NETWORK;
    uint64_t COMPARA = GEOPM_REGION_HINT_COMPUTE | GEOPM_REGION_HINT_PARALLEL;

    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NOHINTS, m_status->get_hint(1));
    EXPECT_EQ(NOHINTS, m_status->get_hint(2));
    EXPECT_EQ(NOHINTS, m_status->get_hint(3));

    m_status->set_hint(1, NETWORK);
    m_status->set_hint(3, NETWORK);
    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NETWORK, m_status->get_hint(1));
    EXPECT_EQ(NOHINTS, m_status->get_hint(2));
    EXPECT_EQ(NETWORK, m_status->get_hint(3));

    m_status->set_hint(2, COMPARA);
    m_status->set_hint(3, COMPARA);
    EXPECT_EQ(NOHINTS, m_status->get_hint(0));
    EXPECT_EQ(NETWORK, m_status->get_hint(1));
    EXPECT_EQ(COMPARA, m_status->get_hint(2));
    EXPECT_EQ(COMPARA, m_status->get_hint(3));

    // clear hint
    m_status->set_hint(1, 0ULL);
    m_status->set_hint(2, 0ULL);
    m_status->set_hint(3, 0ULL);
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
}

TEST_F(ApplicationStatusTest, hash)
{
    ASSERT_EQ(0x0, GEOPM_REGION_HASH_INVALID);

    EXPECT_EQ(0x00, m_status->get_hash(0));
    EXPECT_EQ(0x00, m_status->get_hash(1));
    EXPECT_EQ(0x00, m_status->get_hash(2));
    EXPECT_EQ(0x00, m_status->get_hash(3));

    m_status->set_hash(0, 0xAA);
    m_status->set_hash(1, 0xAA);
    m_status->set_hash(2, 0xBB);
    m_status->set_hash(3, 0xCC);
    EXPECT_EQ(0xAA, m_status->get_hash(0));
    EXPECT_EQ(0xAA, m_status->get_hash(1));
    EXPECT_EQ(0xBB, m_status->get_hash(2));
    EXPECT_EQ(0xCC, m_status->get_hash(3));

    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hash(-1, 0xDD),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hash(99, 0xDD),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->set_hash(0, (0xFFULL << 32)),
                               GEOPM_ERROR_INVALID, "invalid region hash");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hash(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_hash(99),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
}

TEST_F(ApplicationStatusTest, work_progress)
{

    // CPUs 2 and 3 are inactive, 0 work units
    m_status->set_total_work_units(0, 4);
    m_status->set_total_work_units(1, 8);
    EXPECT_DOUBLE_EQ(0.000, m_status->get_work_progress(0));
    EXPECT_DOUBLE_EQ(0.000, m_status->get_work_progress(1));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(2)));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(3)));
    m_status->increment_work_unit(0);
    m_status->increment_work_unit(1);
    EXPECT_DOUBLE_EQ(0.250, m_status->get_work_progress(0));
    EXPECT_DOUBLE_EQ(0.125, m_status->get_work_progress(1));
    m_status->increment_work_unit(0);
    EXPECT_DOUBLE_EQ(0.500, m_status->get_work_progress(0));
    EXPECT_DOUBLE_EQ(0.125, m_status->get_work_progress(1));
    EXPECT_DOUBLE_EQ(0.125, m_status->get_work_progress(1));
    m_status->increment_work_unit(0);
    m_status->increment_work_unit(1);
    EXPECT_DOUBLE_EQ(0.750, m_status->get_work_progress(0));
    EXPECT_DOUBLE_EQ(0.250, m_status->get_work_progress(1));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(2)));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(3)));
    m_status->increment_work_unit(0);
    m_status->increment_work_unit(1);
    m_status->increment_work_unit(1);
    EXPECT_DOUBLE_EQ(1.000, m_status->get_work_progress(0));
    EXPECT_DOUBLE_EQ(0.500, m_status->get_work_progress(1));

    GEOPM_EXPECT_THROW_MESSAGE(m_status->increment_work_unit(0),
                               GEOPM_ERROR_RUNTIME, "more increments than total work");

    // reset progress
    m_status->set_total_work_units(0, 8);
    EXPECT_DOUBLE_EQ(0.00, m_status->get_work_progress(0));

    // leave region
    m_status->set_total_work_units(0, 0);
    m_status->set_total_work_units(1, 0);
    m_status->set_total_work_units(2, 0);
    m_status->set_total_work_units(3, 0);
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(0)));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(1)));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(2)));
    EXPECT_TRUE(std::isnan(m_status->get_work_progress(3)));


    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_work_progress(-1),
                               GEOPM_ERROR_INVALID, "invalid CPU index");
    GEOPM_EXPECT_THROW_MESSAGE(m_status->get_work_progress(99),
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
    EXPECT_EQ(34, m_status->get_process(0));
    EXPECT_EQ(56, m_status->get_process(1));
    EXPECT_EQ(34, m_status->get_process(2));
    EXPECT_EQ(78, m_status->get_process(3));

    // detach processes
    m_status->set_process({0, 1, 2, 3}, -1);
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
