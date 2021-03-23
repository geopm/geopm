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

#include "ApplicationSamplerImp.hpp"
#include "MockProfileSampler.hpp"
#include "MockApplicationRecordLog.hpp"
#include "MockRecordFilter.hpp"
#include "MockApplicationStatus.hpp"
#include "MockPlatformTopo.hpp"
#include "record.hpp"
#include "geopm.h"
#include "geopm_internal.h"
#include "geopm_test.hpp"

using testing::_;
using testing::Return;
using testing::SetArgReferee;
using testing::DoAll;
using geopm::ApplicationSampler;
using geopm::ApplicationSamplerImp;
using geopm::ProfileSampler;
using geopm::record_s;
using geopm::short_region_s;
using geopm::RecordFilter;
using geopm::ApplicationRecordLog;


class ApplicationSamplerTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockProfileSampler> m_mock_profile_sampler;
        std::shared_ptr<ApplicationSampler> m_app_sampler;
        std::map<int, ApplicationSamplerImp::m_process_s> m_process_map;
        std::shared_ptr<MockRecordFilter> m_filter_0;
        std::shared_ptr<MockRecordFilter> m_filter_1;
        std::shared_ptr<MockApplicationRecordLog> m_record_log_0;
        std::shared_ptr<MockApplicationRecordLog> m_record_log_1;
        std::shared_ptr<MockApplicationStatus> m_mock_status;
        int m_num_cpu;
        std::unique_ptr<MockPlatformTopo> m_mock_topo;
};

void ApplicationSamplerTest::SetUp()
{
    m_mock_profile_sampler = std::make_shared<MockProfileSampler>();
    m_filter_0 = std::make_shared<MockRecordFilter>();
    m_filter_1 = std::make_shared<MockRecordFilter>();
    m_record_log_0 = std::make_shared<MockApplicationRecordLog>();
    m_record_log_1 = std::make_shared<MockApplicationRecordLog>();
    m_mock_status = std::make_shared<MockApplicationStatus>();
    m_num_cpu = 2;

    m_process_map[0].filter = m_filter_0;
    m_process_map[0].record_log = m_record_log_0;
    m_process_map[234].filter = m_filter_1;
    m_process_map[234].record_log = m_record_log_1;
    std::vector<bool> is_active(2, true);
    m_mock_topo = geopm::make_unique<MockPlatformTopo>();
    EXPECT_CALL(*m_mock_topo, num_domain(GEOPM_DOMAIN_CPU))
        .WillOnce(Return(m_num_cpu));

    m_app_sampler = std::make_shared<ApplicationSamplerImp>(m_mock_status,
                                                            *m_mock_topo,
                                                            m_process_map,
                                                            false,
                                                            "",
                                                            is_active);
    m_app_sampler->set_sampler(m_mock_profile_sampler);
    m_app_sampler->time_zero(geopm_time_s {{0,0}});
}

TEST_F(ApplicationSamplerTest, one_enter_exit)
{
    uint64_t region_hash = 0xabcdULL;
    std::vector<record_s> message_buffer {
    //   time    process    event                      signal
        {10,     0,         geopm::EVENT_REGION_ENTRY, region_hash},
        {11,     0,         geopm::EVENT_REGION_EXIT,  region_hash},
    };
    std::vector<record_s> empty_message_buffer;
    std::vector<short_region_s> empty_short_region_buffer;
    EXPECT_CALL(*m_record_log_0, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer),
                        SetArgReferee<1>(empty_short_region_buffer)));
    EXPECT_CALL(*m_record_log_1, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                        SetArgReferee<1>(empty_short_region_buffer)));
    EXPECT_CALL(*m_mock_status, get_hint(_))
        .WillRepeatedly(Return(GEOPM_REGION_HINT_UNKNOWN));
    EXPECT_CALL(*m_mock_status, update_cache());
    m_app_sampler->update({{1, 0}});
    std::vector<struct record_s> result {
         m_app_sampler->get_records()
    };

    ASSERT_EQ(2U, result.size());

    EXPECT_EQ(10.0, result[0].time);
    EXPECT_EQ(0, result[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(region_hash, result[0].signal);

    EXPECT_EQ(11.0, result[1].time);
    EXPECT_EQ(0, result[1].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[1].event);
    EXPECT_EQ(region_hash, result[1].signal);
}

TEST_F(ApplicationSamplerTest, one_enter_exit_two_ranks)
{
    uint64_t region_hash = 0xabcdULL;
    std::vector<record_s> message_buffer_0 {
    //   time    process    event                      signal
        {10,     0,         geopm::EVENT_REGION_ENTRY, region_hash},
        {11,     0,         geopm::EVENT_REGION_EXIT,  region_hash},
    };
    std::vector<record_s> message_buffer_1 {
    //   time      process      event                      signal
        {10.5,     234,         geopm::EVENT_REGION_ENTRY, region_hash},
        {11.5,     234,         geopm::EVENT_REGION_EXIT,  region_hash},
    };

    std::vector<short_region_s> empty_short_region_buffer;
    EXPECT_CALL(*m_record_log_0, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer_0),
                        SetArgReferee<1>(empty_short_region_buffer)));
    EXPECT_CALL(*m_record_log_1, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer_1),
                        SetArgReferee<1>(empty_short_region_buffer)));
    EXPECT_CALL(*m_mock_status, update_cache());
    m_app_sampler->update({{1, 0}});
    std::vector<struct record_s> result {
         m_app_sampler->get_records()
    };

    ASSERT_EQ(4U, result.size());

    EXPECT_EQ(10.0, result[0].time);
    EXPECT_EQ(0, result[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(region_hash, result[0].signal);

    EXPECT_EQ(11.0, result[1].time);
    EXPECT_EQ(0, result[1].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[1].event);
    EXPECT_EQ(region_hash, result[1].signal);

    EXPECT_EQ(10.5, result[2].time);
    EXPECT_EQ(234, result[2].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[2].event);
    EXPECT_EQ(region_hash, result[2].signal);

    EXPECT_EQ(11.5, result[3].time);
    EXPECT_EQ(234, result[3].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[3].event);
    EXPECT_EQ(region_hash, result[3].signal);
}

TEST_F(ApplicationSamplerTest, with_epoch)
{
    uint64_t region_hash_0 = 0xabcdULL;
    uint64_t region_hash_1 = 0x1234ULL;

    std::vector<record_s> message_buffer_0 {
    //   time      process      event                      signal
        {10.0,     0,           geopm::EVENT_REGION_ENTRY, region_hash_0},
        {11.0,     0,           geopm::EVENT_EPOCH_COUNT,  1},
        {12.0,     0,           geopm::EVENT_REGION_EXIT, region_hash_0},
        {13.0,     0,           geopm::EVENT_REGION_ENTRY, region_hash_1},
        {14.0,     0,           geopm::EVENT_EPOCH_COUNT, 2},
        {15.0,     0,           geopm::EVENT_REGION_EXIT, region_hash_1},
    };

    std::vector<record_s> message_buffer_1 {
    //   time      process      event                      signal
        {10.5,     234,         geopm::EVENT_REGION_ENTRY, region_hash_0},
        {11.5,     234,         geopm::EVENT_EPOCH_COUNT,  1},
        {12.5,     234,         geopm::EVENT_REGION_EXIT, region_hash_0},
        {13.5,     234,         geopm::EVENT_REGION_ENTRY, region_hash_1},
        {14.5,     234,         geopm::EVENT_EPOCH_COUNT, 2},
        {15.5,     234,         geopm::EVENT_REGION_EXIT, region_hash_1},
    };

    std::vector<short_region_s> empty_short_region_buffer;
    EXPECT_CALL(*m_record_log_0, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer_0),
                        SetArgReferee<1>(empty_short_region_buffer)));
    EXPECT_CALL(*m_record_log_1, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer_1),
                        SetArgReferee<1>(empty_short_region_buffer)));
    EXPECT_CALL(*m_mock_status, update_cache());
    m_app_sampler->update({{1, 0}});
    std::vector<struct record_s> result {
         m_app_sampler->get_records()
    };

    ASSERT_EQ(12U, result.size());

    EXPECT_EQ(10.0, result[0].time);
    EXPECT_EQ(0, result[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(region_hash_0, result[0].signal);

    EXPECT_EQ(11.0, result[1].time);
    EXPECT_EQ(0, result[1].process);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
    EXPECT_EQ(1U, result[1].signal);

    EXPECT_EQ(12.0, result[2].time);
    EXPECT_EQ(0, result[2].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[2].event);
    EXPECT_EQ(region_hash_0, result[2].signal);

    EXPECT_EQ(13.0, result[3].time);
    EXPECT_EQ(0, result[3].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[3].event);
    EXPECT_EQ(region_hash_1, result[3].signal);

    EXPECT_EQ(14.0, result[4].time);
    EXPECT_EQ(0, result[4].process);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[4].event);
    EXPECT_EQ(2U, result[4].signal);

    EXPECT_EQ(15.0, result[5].time);
    EXPECT_EQ(0, result[5].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[5].event);
    EXPECT_EQ(region_hash_1, result[5].signal);

    EXPECT_EQ(10.5, result[6].time);
    EXPECT_EQ(234, result[6].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[6].event);
    EXPECT_EQ(region_hash_0, result[6].signal);

    EXPECT_EQ(11.5, result[7].time);
    EXPECT_EQ(234, result[7].process);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[7].event);
    EXPECT_EQ(1U, result[7].signal);

    EXPECT_EQ(12.5, result[8].time);
    EXPECT_EQ(234, result[8].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[8].event);
    EXPECT_EQ(region_hash_0, result[8].signal);

    EXPECT_EQ(13.5, result[9].time);
    EXPECT_EQ(234, result[9].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[9].event);
    EXPECT_EQ(region_hash_1, result[9].signal);

    EXPECT_EQ(14.5, result[10].time);
    EXPECT_EQ(234, result[10].process);
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[10].event);
    EXPECT_EQ(2U, result[10].signal);

    EXPECT_EQ(15.5, result[11].time);
    EXPECT_EQ(234, result[11].process);
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, result[11].event);
    EXPECT_EQ(region_hash_1, result[11].signal);
}

TEST_F(ApplicationSamplerTest, string_conversion)
{
    EXPECT_EQ("REGION_ENTRY", geopm::event_name(geopm::EVENT_REGION_ENTRY));
    EXPECT_EQ("REGION_EXIT", geopm::event_name(geopm::EVENT_REGION_EXIT));
    EXPECT_EQ("EPOCH_COUNT", geopm::event_name(geopm::EVENT_EPOCH_COUNT));
    EXPECT_EQ("HINT", geopm::event_name(geopm::EVENT_HINT));

    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, geopm::event_type("REGION_ENTRY"));
    EXPECT_EQ(geopm::EVENT_REGION_EXIT, geopm::event_type("REGION_EXIT"));
    EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, geopm::event_type("EPOCH_COUNT"));
    EXPECT_EQ(geopm::EVENT_HINT, geopm::event_type("HINT"));

    EXPECT_THROW(geopm::event_name(99), geopm::Exception);
    EXPECT_THROW(geopm::event_type("INVALID"), geopm::Exception);
}

TEST_F(ApplicationSamplerTest, short_regions)
{
    uint64_t region_hash_0 = 0xabcdULL;
    uint64_t region_hash_1 = 0x1234ULL;
    std::vector<record_s> message_buffer_0 {
    //   time    process    event                      signal
        {10,     0,         geopm::EVENT_SHORT_REGION, 0},
    };
    std::vector<record_s> message_buffer_1 {
        {11,     234,       geopm::EVENT_SHORT_REGION, 0},
    };
    std::vector<short_region_s> short_region_buffer_0 {
    //   hash           num_complete, total_time
        {region_hash_0, 3,             1.0}
    };
    std::vector<short_region_s> short_region_buffer_1 {
    //   hash           num_complete, total_time
        {region_hash_1, 4,            1.1}
    };
    EXPECT_CALL(*m_record_log_0, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer_0),
                        SetArgReferee<1>(short_region_buffer_0)));
    EXPECT_CALL(*m_record_log_1, dump(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(message_buffer_1),
                        SetArgReferee<1>(short_region_buffer_1)));
    EXPECT_CALL(*m_mock_status, update_cache());
    m_app_sampler->update({{1, 0}});
    std::vector<struct record_s> records {
        m_app_sampler->get_records()
    };

    ASSERT_EQ(2U, records.size());

    EXPECT_EQ(10.0, records[0].time);
    EXPECT_EQ(0, records[0].process);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[0].event);
    EXPECT_EQ(0ULL, records[0].signal);

    EXPECT_EQ(11.0, records[1].time);
    EXPECT_EQ(234, records[1].process);
    EXPECT_EQ(geopm::EVENT_SHORT_REGION, records[1].event);
    EXPECT_EQ(1ULL, records[1].signal);

    std::vector<struct short_region_s> short_regions {
        m_app_sampler->get_short_region(0),
        m_app_sampler->get_short_region(1),
    };

    EXPECT_EQ(region_hash_0, short_regions[0].hash);
    EXPECT_EQ(region_hash_1, short_regions[1].hash);
    EXPECT_EQ(3, short_regions[0].num_complete);
    EXPECT_EQ(4, short_regions[1].num_complete);
    EXPECT_EQ(1.0, short_regions[0].total_time);
    EXPECT_EQ(1.1, short_regions[1].total_time);

    GEOPM_EXPECT_THROW_MESSAGE(m_app_sampler->get_short_region(3),
                               GEOPM_ERROR_INVALID,
                               "event_signal does not match any short region handle");
}

TEST_F(ApplicationSamplerTest, hash)
{
    uint64_t region_a = 0xAAAA;
    uint64_t region_b = 0xBBBB;
    EXPECT_CALL(*m_mock_status, get_hash(0))
        .WillOnce(Return(region_a));
    EXPECT_CALL(*m_mock_status, get_hash(1))
        .WillOnce(Return(region_b));
    uint64_t hash = m_app_sampler->cpu_region_hash(0);
    EXPECT_EQ(region_a, hash);
    hash = m_app_sampler->cpu_region_hash(1);
    EXPECT_EQ(region_b, hash);
}

TEST_F(ApplicationSamplerTest, hint)
{
    EXPECT_CALL(*m_mock_status, get_hint(0))
        .WillOnce(Return(GEOPM_REGION_HINT_COMPUTE));
    uint64_t hint = m_app_sampler->cpu_hint(0);
    EXPECT_EQ(GEOPM_REGION_HINT_COMPUTE, hint);
    EXPECT_CALL(*m_mock_status, get_hint(1))
        .WillOnce(Return(GEOPM_REGION_HINT_MEMORY));
    hint = m_app_sampler->cpu_hint(1);
    EXPECT_EQ(GEOPM_REGION_HINT_MEMORY, hint);
}

TEST_F(ApplicationSamplerTest, hint_time)
{
    double compute_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(0.0, compute_time);
    double network_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(0.0, network_time);
    double memory_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    compute_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(0.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(0.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    std::vector<record_s> empty_message_buffer;
    std::vector<short_region_s> empty_short_region_buffer;
    {
        EXPECT_CALL(*m_record_log_0, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_record_log_1, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_mock_status, update_cache());
        EXPECT_CALL(*m_mock_status, get_hint(_))
            .WillOnce(Return(GEOPM_REGION_HINT_NETWORK))
            .WillOnce(Return(GEOPM_REGION_HINT_COMPUTE));
        m_app_sampler->update({{1, 0}});
    }
    compute_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(0.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(0.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    compute_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(0.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(0.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    {
        EXPECT_CALL(*m_record_log_0, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_record_log_1, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_mock_status, update_cache());
        EXPECT_CALL(*m_mock_status, get_hint(_))
            .WillOnce(Return(GEOPM_REGION_HINT_NETWORK))
            .WillOnce(Return(GEOPM_REGION_HINT_MEMORY));
        m_app_sampler->update({{2, 0}});
    }
    compute_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(0.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(1.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    compute_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(1.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(0.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    {
        EXPECT_CALL(*m_record_log_0, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_record_log_1, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_mock_status, update_cache());
        EXPECT_CALL(*m_mock_status, get_hint(_))
            .WillOnce(Return(GEOPM_REGION_HINT_COMPUTE))
            .WillOnce(Return(GEOPM_REGION_HINT_NETWORK));
        m_app_sampler->update({{4, 0}});
    }
    compute_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(0.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(3.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    compute_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(1.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(0.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(2.0, memory_time);
    {
        EXPECT_CALL(*m_record_log_0, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_record_log_1, dump(_, _))
            .WillOnce(DoAll(SetArgReferee<0>(empty_message_buffer),
                            SetArgReferee<1>(empty_short_region_buffer)));
        EXPECT_CALL(*m_mock_status, update_cache());
        EXPECT_CALL(*m_mock_status, get_hint(_))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET))
            .WillOnce(Return(GEOPM_REGION_HINT_UNSET));
        m_app_sampler->update({{7, 0}});
    }
    compute_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(3.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(3.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(0, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(0.0, memory_time);
    compute_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_COMPUTE);
    EXPECT_EQ(1.0, compute_time);
    network_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_NETWORK);
    EXPECT_EQ(3.0, network_time);
    memory_time = m_app_sampler->cpu_hint_time(1, GEOPM_REGION_HINT_MEMORY);
    EXPECT_EQ(2.0, memory_time);
}

TEST_F(ApplicationSamplerTest, cpu_process)
{
    // TODO: re-enable with fix to per_cpu_process implementation
    /*
    EXPECT_CALL(*m_mock_status, get_process(_))
        .WillOnce(Return(42))
        .WillOnce(Return(24));
    std::vector<int> cpu_process = m_app_sampler->per_cpu_process();
    std::vector<int> expected {42, 24};
    EXPECT_EQ(expected, cpu_process);
    */
}

TEST_F(ApplicationSamplerTest, cpu_progress)
{
    double expected = 0.75;
    EXPECT_CALL(*m_mock_status, get_progress_cpu(1))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, m_app_sampler->cpu_progress(1));
}
