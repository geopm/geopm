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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "ApplicationSampler.hpp"
#include "MockProfileSampler.hpp"
#include "MockEpochRuntimeRegulator.hpp"
#include "geopm.h"
#include "geopm_internal.h"

using testing::_;
using testing::Return;
using geopm::ApplicationSampler;
using geopm::ProfileSampler;
using geopm::EpochRuntimeRegulator;


class ApplicationSamplerTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockProfileSampler> m_mock_profile_sampler;
        std::shared_ptr<MockEpochRuntimeRegulator> m_mock_regulator;
};

void ApplicationSamplerTest::SetUp()
{
    m_mock_profile_sampler = std::make_shared<MockProfileSampler>();
    m_mock_regulator = std::make_shared<MockEpochRuntimeRegulator>();
    EXPECT_CALL(*m_mock_profile_sampler, capacity())
        .WillOnce(Return(4096));
    ApplicationSampler::application_sampler().set_sampler(m_mock_profile_sampler);
    ApplicationSampler::application_sampler().set_regulator(m_mock_regulator);
}

TEST_F(ApplicationSamplerTest, one_enter_exit)
{
    uint64_t region_id = 0xabcdULL | GEOPM_REGION_HINT_COMPUTE;
    std::vector<struct geopm_prof_message_s> message_buffer {
    //   rank     region_id     timestamp     progress
        {0,       region_id,    {{10,0}},     0.0},
        {0,       region_id,    {{11,0}},     1.0},
    };
    EXPECT_CALL(*m_mock_profile_sampler, sample_cache())
        .WillOnce(Return(message_buffer));
    ApplicationSampler::application_sampler().update_records();
    std::vector<struct ApplicationSampler::m_record_s> result {
        ApplicationSampler::application_sampler().get_records()
    };

    ASSERT_EQ(4U, result.size());

    EXPECT_EQ(10.0, result[0].time);
    EXPECT_EQ(0, result[0].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(0xabcdULL, result[0].signal);

    EXPECT_EQ(10.0, result[1].time);
    EXPECT_EQ(0, result[1].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[1].event);
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, result[1].signal);

    EXPECT_EQ(11.0, result[2].time);
    EXPECT_EQ(0, result[2].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_EXIT, result[2].event);
    EXPECT_EQ(0xabcdULL, result[2].signal);

    EXPECT_EQ(11.0, result[3].time);
    EXPECT_EQ(0, result[3].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[3].event);
    EXPECT_EQ(GEOPM_REGION_HINT_UNKNOWN, result[3].signal);
}

TEST_F(ApplicationSamplerTest, with_mpi)
{
    uint64_t region_id = 0xabcdULL | GEOPM_REGION_HINT_COMPUTE;
    uint64_t mpi_id = geopm_region_id_set_mpi(region_id);
    std::vector<struct geopm_prof_message_s> message_buffer {
    //   rank     region_id     timestamp     progress
        {234,     region_id,    {{10,0}},     0.0},
        {234,     mpi_id,       {{11,0}},     0.0},
        {234,     mpi_id,       {{12,0}},     1.0},
        {234,     region_id,    {{13,0}},     1.0},
    };
    EXPECT_CALL(*m_mock_profile_sampler, sample_cache())
        .WillOnce(Return(message_buffer));
    ApplicationSampler::application_sampler().update_records();
    std::vector<struct ApplicationSampler::m_record_s> result {
        ApplicationSampler::application_sampler().get_records()
    };

    ASSERT_EQ(6U, result.size());

    EXPECT_EQ(10.0, result[0].time);
    EXPECT_EQ(234, result[0].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(0xabcdULL, result[0].signal);

    EXPECT_EQ(10.0, result[1].time);
    EXPECT_EQ(234, result[1].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[1].event);
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, result[1].signal);

    EXPECT_EQ(11.0, result[2].time);
    EXPECT_EQ(234, result[2].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[2].event);
    EXPECT_EQ(GEOPM_REGION_HINT_NETWORK, result[2].signal);

    EXPECT_EQ(12.0, result[3].time);
    EXPECT_EQ(234, result[3].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[3].event);
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, result[3].signal);

    EXPECT_EQ(13.0, result[4].time);
    EXPECT_EQ(234, result[4].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_EXIT, result[4].event);
    EXPECT_EQ(0xabcdULL, result[4].signal);

    EXPECT_EQ(13.0, result[5].time);
    EXPECT_EQ(234, result[5].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[5].event);
    EXPECT_EQ(GEOPM_REGION_HINT_UNKNOWN, result[5].signal);
}

TEST_F(ApplicationSamplerTest, with_epoch)
{
    uint64_t region_id = 0xabcdULL | GEOPM_REGION_HINT_COMPUTE;
    uint64_t epoch_id = GEOPM_REGION_ID_EPOCH;
    std::vector<struct geopm_prof_message_s> message_buffer {
    //   rank     region_id     timestamp     progress
        {0,       region_id,    {{10,0}},     0.0},
        {0,       epoch_id,     {{11,0}},     0.0},
        {0,       region_id,    {{12,0}},     1.0},
        {0,       region_id,    {{13,0}},     0.0},
        {0,       epoch_id,     {{14,0}},     0.0},
        {0,       region_id,    {{15,0}},     1.0},
    };
    EXPECT_CALL(*m_mock_profile_sampler, sample_cache())
        .WillOnce(Return(message_buffer));
    ApplicationSampler::application_sampler().update_records();
    std::vector<struct ApplicationSampler::m_record_s> result {
        ApplicationSampler::application_sampler().get_records()
    };

    ASSERT_EQ(10U, result.size());

    EXPECT_EQ(10.0, result[0].time);
    EXPECT_EQ(0, result[0].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(0xabcdULL, result[0].signal);

    EXPECT_EQ(10.0, result[1].time);
    EXPECT_EQ(0, result[1].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[1].event);
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, result[1].signal);

    EXPECT_EQ(11.0, result[2].time);
    EXPECT_EQ(0, result[2].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_EPOCH_COUNT, result[2].event);
    EXPECT_EQ(1ULL, result[2].signal);

    EXPECT_EQ(12.0, result[3].time);
    EXPECT_EQ(0, result[3].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_EXIT, result[3].event);
    EXPECT_EQ(0xabcdULL, result[3].signal);

    EXPECT_EQ(12.0, result[4].time);
    EXPECT_EQ(0, result[4].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[4].event);
    EXPECT_EQ(GEOPM_REGION_HINT_UNKNOWN, result[4].signal);

    EXPECT_EQ(13.0, result[5].time);
    EXPECT_EQ(0, result[5].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_ENTRY, result[5].event);
    EXPECT_EQ(0xabcdULL, result[5].signal);

    EXPECT_EQ(13.0, result[6].time);
    EXPECT_EQ(0, result[6].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[6].event);
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, result[6].signal);

    EXPECT_EQ(14.0, result[7].time);
    EXPECT_EQ(0, result[7].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_EPOCH_COUNT, result[7].event);
    EXPECT_EQ(2ULL, result[7].signal);

    EXPECT_EQ(15.0, result[8].time);
    EXPECT_EQ(0, result[8].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_REGION_EXIT, result[8].event);
    EXPECT_EQ(0xabcdULL, result[8].signal);

    EXPECT_EQ(15.0, result[9].time);
    EXPECT_EQ(0, result[9].process);
    EXPECT_EQ(ApplicationSampler::M_EVENT_HINT, result[9].event);
    EXPECT_EQ(GEOPM_REGION_HINT_UNKNOWN, result[9].signal);
}
