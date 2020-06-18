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

#include <cstdint>
#include "gtest/gtest.h"
#include "geopm_test.hpp"

#include "ProxyEpochRecordFilter.hpp"
#include "record.hpp"
#include "MockApplicationSampler.hpp"

using geopm::record_s;
using geopm::ProxyEpochRecordFilter;

class ProxyEpochRecordFilterTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::vector<int> m_in_events;
        std::vector<int> m_out_events;
};


void ProxyEpochRecordFilterTest::SetUp()
{
    m_in_events = {
        geopm::EVENT_HINT,
        geopm::EVENT_REGION_ENTRY,
        geopm::EVENT_REGION_EXIT,
        geopm::EVENT_PROFILE,
        geopm::EVENT_REPORT,
        geopm::EVENT_CLAIM_CPU,
        geopm::EVENT_RELEASE_CPU,
        geopm::EVENT_NAME_KEY,
    };
    m_out_events = {
        geopm::EVENT_EPOCH_COUNT,
    };
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
    const std::string tutorial_2_prof_trace = R"(# geopm_version: 1.1.0+dev241g195e2ed
# start_time: Tue Jun 16 17:28:33 2020
# profile_name: /home/cmcantal/.local/tmp/geopm-tutorial-test.Y4tAoMq7/geopm-tutorial/./tutorial_2
# node_name: mcfly1
# agent: monitor
TIME|PROCESS|EVENT|SIGNAL
0.307342336|0|EPOCH_COUNT|1
0.307344815|0|REGION_ENTRY|0x000000009803a79a
0.307344815|0|HINT|UNKNOWN
1.308838429|0|REGION_EXIT|0x000000009803a79a
1.308838429|0|HINT|UNKNOWN
1.308840488|0|REGION_ENTRY|0x000000008a1e10cd
1.308840488|0|HINT|MEMORY
1.851147587|0|REGION_EXIT|0x000000008a1e10cd
1.851147587|0|HINT|UNKNOWN
1.851151005|0|REGION_ENTRY|0x00000000c82f28bd
1.851151005|0|HINT|COMPUTE
5.447514411|0|REGION_EXIT|0x00000000c82f28bd
5.447514411|0|HINT|UNKNOWN
5.447570864|0|REGION_ENTRY|0x000000008a1e10cd
5.447570864|0|HINT|MEMORY
5.913116843|0|REGION_EXIT|0x000000008a1e10cd
5.913116843|0|HINT|UNKNOWN
5.913121481|0|REGION_ENTRY|0x00000000d88e02a5
5.913121481|0|HINT|NETWORK
5.992377296|0|REGION_EXIT|0x00000000d88e02a5
5.992377296|0|HINT|UNKNOWN
5.992442871|0|EPOCH_COUNT|2
5.992443772|0|REGION_ENTRY|0x000000009803a79a
5.992443772|0|HINT|UNKNOWN
6.994770995|0|REGION_EXIT|0x000000009803a79a
6.994770995|0|HINT|UNKNOWN
6.994775322|0|REGION_ENTRY|0x000000008a1e10cd
6.994775322|0|HINT|MEMORY
7.452432595|0|REGION_EXIT|0x000000008a1e10cd
7.452432595|0|HINT|UNKNOWN
7.452436065|0|REGION_ENTRY|0x00000000c82f28bd
7.452436065|0|HINT|COMPUTE
8.921080448|0|REGION_EXIT|0x00000000c82f28bd
8.921080448|0|HINT|UNKNOWN
8.921087924|0|REGION_ENTRY|0x000000008a1e10cd
8.921087924|0|HINT|MEMORY
9.386760427|0|REGION_EXIT|0x000000008a1e10cd
9.386760427|0|HINT|UNKNOWN
9.386763809|0|REGION_ENTRY|0x00000000d88e02a5
9.386763809|0|HINT|NETWORK
9.397085301000001|0|REGION_EXIT|0x00000000d88e02a5
9.397085301000001|0|HINT|UNKNOWN
9.397107401|0|EPOCH_COUNT|3
9.397108303|0|REGION_ENTRY|0x000000009803a79a
9.397108303|0|HINT|UNKNOWN
10.399224657|0|REGION_EXIT|0x000000009803a79a
10.399224657|0|HINT|UNKNOWN
10.39922937|0|REGION_ENTRY|0x000000008a1e10cd
10.39922937|0|HINT|MEMORY
10.866043778|0|REGION_EXIT|0x000000008a1e10cd
10.866043778|0|HINT|UNKNOWN
10.866046929|0|REGION_ENTRY|0x00000000c82f28bd
10.866046929|0|HINT|COMPUTE
12.369165299|0|REGION_EXIT|0x00000000c82f28bd
12.369165299|0|HINT|UNKNOWN
12.369173539|0|REGION_ENTRY|0x000000008a1e10cd
12.369173539|0|HINT|MEMORY
12.835004381|0|REGION_EXIT|0x000000008a1e10cd
12.835004381|0|HINT|UNKNOWN
12.835008532|0|REGION_ENTRY|0x00000000d88e02a5
12.835008532|0|HINT|NETWORK
12.835348127|0|REGION_EXIT|0x00000000d88e02a5
12.835348127|0|HINT|UNKNOWN
12.835370458|0|EPOCH_COUNT|4
12.8353714|0|REGION_ENTRY|0x000000009803a79a
12.8353714|0|HINT|UNKNOWN
13.837451023|0|REGION_EXIT|0x000000009803a79a
13.837451023|0|HINT|UNKNOWN
13.837455413|0|REGION_ENTRY|0x000000008a1e10cd
13.837455413|0|HINT|MEMORY
14.301486971|0|REGION_EXIT|0x000000008a1e10cd
14.301486971|0|HINT|UNKNOWN
14.301552592|0|REGION_ENTRY|0x00000000c82f28bd
14.301552592|0|HINT|COMPUTE
15.791350535|0|REGION_EXIT|0x00000000c82f28bd
15.791350535|0|HINT|UNKNOWN
15.791354529|0|REGION_ENTRY|0x000000008a1e10cd
15.791354529|0|HINT|MEMORY
16.266415946|0|REGION_EXIT|0x000000008a1e10cd
16.266415946|0|HINT|UNKNOWN
16.266420042|0|REGION_ENTRY|0x00000000d88e02a5
16.266420042|0|HINT|NETWORK
16.266821879|0|REGION_EXIT|0x00000000d88e02a5
16.266821879|0|HINT|UNKNOWN
16.266843102|0|EPOCH_COUNT|5
16.266844065|0|REGION_ENTRY|0x000000009803a79a
16.266844065|0|HINT|UNKNOWN
17.267953504|0|REGION_EXIT|0x000000009803a79a
17.267953504|0|HINT|UNKNOWN
17.267955315|0|REGION_ENTRY|0x000000008a1e10cd
17.267955315|0|HINT|MEMORY
17.736667561|0|REGION_EXIT|0x000000008a1e10cd
17.736667561|0|HINT|UNKNOWN
17.736671753|0|REGION_ENTRY|0x00000000c82f28bd
17.736671753|0|HINT|COMPUTE
19.231691108|0|REGION_EXIT|0x00000000c82f28bd
19.231691108|0|HINT|UNKNOWN
19.231698777|0|REGION_ENTRY|0x000000008a1e10cd
19.231698777|0|HINT|MEMORY
19.691851618|0|REGION_EXIT|0x000000008a1e10cd
19.691851618|0|HINT|UNKNOWN
19.691856056|0|REGION_ENTRY|0x00000000d88e02a5
19.691856056|0|HINT|NETWORK
19.692071389|0|REGION_EXIT|0x00000000d88e02a5
19.692071389|0|HINT|UNKNOWN
19.692159858|0|EPOCH_COUNT|6
19.692161053|0|REGION_ENTRY|0x000000009803a79a
19.692161053|0|HINT|UNKNOWN
20.694242515|0|REGION_EXIT|0x000000009803a79a
20.694242515|0|HINT|UNKNOWN
20.694247102|0|REGION_ENTRY|0x000000008a1e10cd
20.694247102|0|HINT|MEMORY
21.158930126|0|REGION_EXIT|0x000000008a1e10cd
21.158930126|0|HINT|UNKNOWN
21.158934213|0|REGION_ENTRY|0x00000000c82f28bd
21.158934213|0|HINT|COMPUTE
22.659703445|0|REGION_EXIT|0x00000000c82f28bd
22.659703445|0|HINT|UNKNOWN
22.659711267|0|REGION_ENTRY|0x000000008a1e10cd
22.659711267|0|HINT|MEMORY
23.133141946|0|REGION_EXIT|0x000000008a1e10cd
23.133141946|0|HINT|UNKNOWN
23.133145611|0|REGION_ENTRY|0x00000000d88e02a5
23.133145611|0|HINT|NETWORK
23.133438917|0|REGION_EXIT|0x00000000d88e02a5
23.133438917|0|HINT|UNKNOWN
23.133465719|0|EPOCH_COUNT|7
23.133466679|0|REGION_ENTRY|0x000000009803a79a
23.133466679|0|HINT|UNKNOWN
24.135386798|0|REGION_EXIT|0x000000009803a79a
24.135386798|0|HINT|UNKNOWN
24.135391008|0|REGION_ENTRY|0x000000008a1e10cd
24.135391008|0|HINT|MEMORY
24.606635412|0|REGION_EXIT|0x000000008a1e10cd
24.606635412|0|HINT|UNKNOWN
24.60663892|0|REGION_ENTRY|0x00000000c82f28bd
24.60663892|0|HINT|COMPUTE
26.104318859|0|REGION_EXIT|0x00000000c82f28bd
26.104318859|0|HINT|UNKNOWN
26.104324149|0|REGION_ENTRY|0x000000008a1e10cd
26.104324149|0|HINT|MEMORY
26.573771332|0|REGION_EXIT|0x000000008a1e10cd
26.573771332|0|HINT|UNKNOWN
26.573775147|0|REGION_ENTRY|0x00000000d88e02a5
26.573775147|0|HINT|NETWORK
26.573974289|0|REGION_EXIT|0x00000000d88e02a5
26.573974289|0|HINT|UNKNOWN
26.573997141|0|EPOCH_COUNT|8
26.573998308|0|REGION_ENTRY|0x000000009803a79a
26.573998308|0|HINT|UNKNOWN
27.574462477|0|REGION_EXIT|0x000000009803a79a
27.574462477|0|HINT|UNKNOWN
27.5744672|0|REGION_ENTRY|0x000000008a1e10cd
27.5744672|0|HINT|MEMORY
28.037618119|0|REGION_EXIT|0x000000008a1e10cd
28.037618119|0|HINT|UNKNOWN
28.037621764|0|REGION_ENTRY|0x00000000c82f28bd
28.037621764|0|HINT|COMPUTE
29.537201883|0|REGION_EXIT|0x00000000c82f28bd
29.537201883|0|HINT|UNKNOWN
29.537209366|0|REGION_ENTRY|0x000000008a1e10cd
29.537209366|0|HINT|MEMORY
29.997261223|0|REGION_EXIT|0x000000008a1e10cd
29.997261223|0|HINT|UNKNOWN
29.997265915|0|REGION_ENTRY|0x00000000d88e02a5
29.997265915|0|HINT|NETWORK
29.997517769|0|REGION_EXIT|0x00000000d88e02a5
29.997517769|0|HINT|UNKNOWN
29.997540459|0|EPOCH_COUNT|9
29.997541742|0|REGION_ENTRY|0x000000009803a79a
29.997541742|0|HINT|UNKNOWN
30.998347826|0|REGION_EXIT|0x000000009803a79a
30.998347826|0|HINT|UNKNOWN
30.9983524|0|REGION_ENTRY|0x000000008a1e10cd
30.9983524|0|HINT|MEMORY
31.4588521|0|REGION_EXIT|0x000000008a1e10cd
31.4588521|0|HINT|UNKNOWN
31.458855928|0|REGION_ENTRY|0x00000000c82f28bd
31.458855928|0|HINT|COMPUTE
32.961073933|0|REGION_EXIT|0x00000000c82f28bd
32.961073933|0|HINT|UNKNOWN
32.961077779|0|REGION_ENTRY|0x000000008a1e10cd
32.961077779|0|HINT|MEMORY
33.433840874|0|REGION_EXIT|0x000000008a1e10cd
33.433840874|0|HINT|UNKNOWN
33.433844754|0|REGION_ENTRY|0x00000000d88e02a5
33.433844754|0|HINT|NETWORK
33.434032849|0|REGION_EXIT|0x00000000d88e02a5
33.434032849|0|HINT|UNKNOWN
33.434055302|0|EPOCH_COUNT|10
33.434056215|0|REGION_ENTRY|0x000000009803a79a
33.434056215|0|HINT|UNKNOWN
34.436156113|0|REGION_EXIT|0x000000009803a79a
34.436156113|0|HINT|UNKNOWN
34.436160403|0|REGION_ENTRY|0x000000008a1e10cd
34.436160403|0|HINT|MEMORY
34.897239396|0|REGION_EXIT|0x000000008a1e10cd
34.897239396|0|HINT|UNKNOWN
34.897244059|0|REGION_ENTRY|0x00000000c82f28bd
34.897244059|0|HINT|COMPUTE
36.392664142|0|REGION_EXIT|0x00000000c82f28bd
36.392664142|0|HINT|UNKNOWN
36.392670902|0|REGION_ENTRY|0x000000008a1e10cd
36.392670902|0|HINT|MEMORY
36.855049823|0|REGION_EXIT|0x000000008a1e10cd
36.855049823|0|HINT|UNKNOWN
36.855053916|0|REGION_ENTRY|0x00000000d88e02a5
36.855053916|0|HINT|NETWORK
36.855350322|0|REGION_EXIT|0x00000000d88e02a5
36.855350322|0|HINT|UNKNOWN
)";
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
