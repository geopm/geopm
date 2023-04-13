/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <cstdint>
#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "ProxyEpochRecordFilter.hpp"
#include "record.hpp"
#include "MockApplicationSampler.hpp"
#include "geopm/Helper.hpp"

using geopm::record_s;
using geopm::ProxyEpochRecordFilter;

class ProxyEpochRecordFilterTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::vector<int> m_in_events;
        std::vector<int> m_out_events;
        std::string m_tutorial_2_prof_trace_path;
};


void ProxyEpochRecordFilterTest::SetUp()
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
    m_tutorial_2_prof_trace_path = GEOPM_SOURCE_DIR;
    m_tutorial_2_prof_trace_path += "/test/ProxyEpochRecordFilterTest.tutorial_2_profile_trace";
}

TEST_F(ProxyEpochRecordFilterTest, simple_conversion)
{
    uint64_t hash = 0xAULL;
    record_s record {0.0,
                     0,
                     geopm::EVENT_REGION_ENTRY,
                     hash};
    geopm::ProxyEpochRecordFilter perf(hash, 1, 0);
    for (uint64_t count = 1; count <= 10; ++count) {
        std::vector<record_s> result = perf.filter(record);
        ASSERT_EQ(2ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
        EXPECT_EQ(0.0, result[1].time);
        EXPECT_EQ(0, result[1].process);
        EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
        EXPECT_EQ(count, result[1].signal);
    }
}

TEST_F(ProxyEpochRecordFilterTest, skip_one)
{
    uint64_t hash = 0xAULL;
    record_s record {0.0,
                     0,
                     geopm::EVENT_REGION_ENTRY,
                     hash};
    geopm::ProxyEpochRecordFilter perf(hash, 2, 0);
    for (uint64_t count = 1; count <= 10; ++count) {
        std::vector<record_s> result = perf.filter(record);
        ASSERT_EQ(2ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
        EXPECT_EQ(0.0, result[1].time);
        EXPECT_EQ(0, result[1].process);
        EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
        EXPECT_EQ(count, result[1].signal);
        result = perf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
    }
}


TEST_F(ProxyEpochRecordFilterTest, skip_two_off_one)
{
    uint64_t hash = 0xAULL;
    record_s record {0.0,
                     0,
                     geopm::EVENT_REGION_ENTRY,
                     hash};
    geopm::ProxyEpochRecordFilter perf(hash, 3, 1);
    std::vector<record_s> result = perf.filter(record);
    ASSERT_EQ(1ULL, result.size());
    EXPECT_EQ(0.0, result[0].time);
    EXPECT_EQ(0, result[0].process);
    EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
    EXPECT_EQ(hash, result[0].signal);
    for (uint64_t count = 1; count <= 10; ++count) {
        result = perf.filter(record);
        ASSERT_EQ(2ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
        EXPECT_EQ(0.0, result[1].time);
        EXPECT_EQ(0, result[1].process);
        EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, result[1].event);
        EXPECT_EQ(count, result[1].signal);
        result = perf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
        result = perf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(geopm::EVENT_REGION_ENTRY, result[0].event);
        EXPECT_EQ(hash, result[0].signal);
    }
}


TEST_F(ProxyEpochRecordFilterTest, invalid_construct)
{
    GEOPM_EXPECT_THROW_MESSAGE(geopm::ProxyEpochRecordFilter perf(~0ULL, 0, 0),
                               GEOPM_ERROR_INVALID, "region_hash");
    GEOPM_EXPECT_THROW_MESSAGE(geopm::ProxyEpochRecordFilter perf(0xAULL, 0, 0),
                               GEOPM_ERROR_INVALID, "calls_per_epoch");
    GEOPM_EXPECT_THROW_MESSAGE(geopm::ProxyEpochRecordFilter perf(0xAULL, -1, 0),
                               GEOPM_ERROR_INVALID, "calls_per_epoch");
    GEOPM_EXPECT_THROW_MESSAGE(geopm::ProxyEpochRecordFilter perf(0xAULL, 1, -1),
                               GEOPM_ERROR_INVALID, "startup_count");
}


TEST_F(ProxyEpochRecordFilterTest, filter_in)
{
    record_s record {};
    geopm::ProxyEpochRecordFilter perf(0xAULL, 1, 0);
    for (auto event : m_in_events) {
        record.event = event;
        std::vector<record_s> result = perf.filter(record);
        ASSERT_EQ(1ULL, result.size());
        EXPECT_EQ(0.0, result[0].time);
        EXPECT_EQ(0, result[0].process);
        EXPECT_EQ(event, result[0].event);
        EXPECT_EQ(0ULL, result[0].signal);
    }
}

TEST_F(ProxyEpochRecordFilterTest, filter_out)
{
    record_s record {};
    std::vector<record_s> result;
    geopm::ProxyEpochRecordFilter perf(0xAULL, 1, 0);
    for (auto event : m_out_events) {
        record.event = event;
        result = perf.filter(record);
        EXPECT_EQ(0ULL, result.size());
    }
}

TEST_F(ProxyEpochRecordFilterTest, parse_name)
{
    uint64_t region_hash = 42ULL;
    int calls_per_epoch = 42;
    int startup_count = 42;
    ProxyEpochRecordFilter::parse_name("proxy_epoch,0xabcd1234",
        region_hash, calls_per_epoch, startup_count);
    EXPECT_EQ(0xabcd1234ULL, region_hash);
    EXPECT_EQ(1, calls_per_epoch);
    EXPECT_EQ(0, startup_count);
    ProxyEpochRecordFilter::parse_name("proxy_epoch,0xabcd1235,10",
        region_hash, calls_per_epoch, startup_count);
    EXPECT_EQ(0xabcd1235ULL, region_hash);
    EXPECT_EQ(10, calls_per_epoch);
    EXPECT_EQ(0, startup_count);
    ProxyEpochRecordFilter::parse_name("proxy_epoch,0xabcd1236,100,1000",
        region_hash, calls_per_epoch, startup_count);
    EXPECT_EQ(0xabcd1236ULL, region_hash);
    EXPECT_EQ(100, calls_per_epoch);
    EXPECT_EQ(1000, startup_count);
    ProxyEpochRecordFilter::parse_name("proxy_epoch,MPI_Barrier,1000,10000",
        region_hash, calls_per_epoch, startup_count);
    EXPECT_EQ(0x7b561f45ULL, region_hash);
    EXPECT_EQ(1000, calls_per_epoch);
    EXPECT_EQ(10000, startup_count);

    GEOPM_EXPECT_THROW_MESSAGE(ProxyEpochRecordFilter::parse_name("not_proxy_epoch",
        region_hash, calls_per_epoch, startup_count),
        GEOPM_ERROR_INVALID, "Expected name of the form");
    GEOPM_EXPECT_THROW_MESSAGE(ProxyEpochRecordFilter::parse_name("proxy_epoch",
        region_hash, calls_per_epoch, startup_count),
        GEOPM_ERROR_INVALID, "requires a hash");
    GEOPM_EXPECT_THROW_MESSAGE(ProxyEpochRecordFilter::parse_name("proxy_epoch,",
        region_hash, calls_per_epoch, startup_count),
        GEOPM_ERROR_INVALID, "Parameter region_hash is empty");
    GEOPM_EXPECT_THROW_MESSAGE(ProxyEpochRecordFilter::parse_name("proxy_epoch,0xabcd1237,not_a_number",
        region_hash, calls_per_epoch, startup_count),
        GEOPM_ERROR_INVALID, "Unable to parse parameter calls_per_epoch");
    GEOPM_EXPECT_THROW_MESSAGE(ProxyEpochRecordFilter::parse_name("proxy_epoch,0xabcd1237,2,not_a_number",
        region_hash, calls_per_epoch, startup_count),
        GEOPM_ERROR_INVALID, "Unable to parse parameter startup_count");
}

TEST_F(ProxyEpochRecordFilterTest, parse_tutorial_2)
{
    const std::string tutorial_2_prof_trace = geopm::read_file(m_tutorial_2_prof_trace_path);
    MockApplicationSampler app;
    ProxyEpochRecordFilter perf(0x9803a79a, 1, 0);
    uint64_t epoch_count = 0;
    bool is_epoch = false;
    app.inject_records(tutorial_2_prof_trace);
    for (double time = 0.0; time < 38.0; time += 1.0) {
        app.update_time(time);
        for (const auto &rec_it : app.get_records()) {
            std::vector<geopm::record_s> filtered_rec = perf.filter(rec_it);
            if (rec_it.event == geopm::EVENT_EPOCH_COUNT) {
                EXPECT_TRUE(filtered_rec.empty());
                is_epoch = true;
                ++epoch_count;
            }
            else if (is_epoch) {
                ASSERT_EQ(2ULL, filtered_rec.size());
                EXPECT_EQ(rec_it.time, filtered_rec[0].time);
                EXPECT_EQ(rec_it.process, filtered_rec[0].process);
                EXPECT_EQ(rec_it.event, filtered_rec[0].event);
                EXPECT_EQ(rec_it.signal, filtered_rec[0].signal);
                EXPECT_EQ(rec_it.time, filtered_rec[1].time);
                EXPECT_EQ(rec_it.process, filtered_rec[1].process);
                EXPECT_EQ(geopm::EVENT_EPOCH_COUNT, filtered_rec[1].event);
                EXPECT_EQ(epoch_count, filtered_rec[1].signal);
                is_epoch = false;
            }
            else {
                ASSERT_EQ(1ULL, filtered_rec.size());
                EXPECT_EQ(rec_it.time, filtered_rec[0].time);
                EXPECT_EQ(rec_it.process, filtered_rec[0].process);
                EXPECT_EQ(rec_it.event, filtered_rec[0].event);
                EXPECT_EQ(rec_it.signal, filtered_rec[0].signal);
            }
        }
    }
}
