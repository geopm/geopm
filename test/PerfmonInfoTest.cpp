/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <map>
#include <string>

#include "gtest/gtest.h"

#include "Exception.hpp"
#include "PerfmonInfo.hpp"

using geopm::PerfmonInfo;
using geopm::parse_perfmon;

class PerfmonInfoTest : public ::testing::Test
{
};

TEST_F(PerfmonInfoTest, invalid_json_throws_error)
{
    std::string test_string;
    std::map<std::string, PerfmonInfo> result;

    // empty string
    test_string = "";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);

    // syntax errors
    test_string = "{\"key\":1";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);
    test_string = "{\"key\":,}";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);
    test_string = "{[{]}}";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);

    // must contain an array of obj
    test_string = "{}";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);
    test_string = "{\"key\":1}";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);
    test_string = "[\"key\"]";
    EXPECT_THROW(parse_perfmon(test_string), geopm::Exception);

    // empty valid json
    test_string = "[]";
    EXPECT_NO_THROW(result = parse_perfmon(test_string));
    EXPECT_EQ(0u, result.size());
}

TEST_F(PerfmonInfoTest, missing_required_key_skips_item)
{
    std::string test_string;
    std::map<std::string, PerfmonInfo> result;

    // missing EventCode
    test_string = "[{"
        "\"UMask\": \"0x04\","
        "\"EventName\": \"TEST_EVENT\","
        "\"Offcore\": \"0\","
        "\"other\": \"ignored\""
        "}]";
    EXPECT_NO_THROW(result = parse_perfmon(test_string));
    EXPECT_EQ(0u, result.size());

    // missing UMask
    test_string = "[{"
        "\"EventCode\": \"0xD2\","
        "\"EventName\": \"TEST_EVENT\","
        "\"Offcore\": \"0\","
        "\"other\": \"ignored\""
        "}]";
    EXPECT_NO_THROW(result = parse_perfmon(test_string));
    EXPECT_EQ(0u, result.size());

    // missing EventName
    test_string = "[{"
        "\"EventCode\": \"0xD2\","
        "\"UMask\": \"0x04\","
        "\"Offcore\": \"0\","
        "\"other\": \"ignored\""
        "}]";
    EXPECT_NO_THROW(result = parse_perfmon(test_string));
    EXPECT_EQ(0u, result.size());

    // missing Offcore
    test_string = "[{"
        "\"EventCode\": \"0xD2\","
        "\"UMask\": \"0x04\","
        "\"EventName\": \"TEST_EVENT\","
        "\"other\": \"ignored\""
        "}]";
    EXPECT_NO_THROW(result = parse_perfmon(test_string));
    EXPECT_EQ(0u, result.size());
}

TEST_F(PerfmonInfoTest, event_fields)
{
    std::string test_string;
    std::map<std::string, PerfmonInfo> result;
    test_string = "[{"
        "\"EventCode\": \"0xD2\","
        "\"UMask\": \"0x04\","
        "\"EventName\": \"TEST_EVENT\","
        "\"Offcore\": \"0\","
        "\"other\": \"ignored\""
        "},{"
        "\"EventCode\": \"0xB7, 0xBB\","
        "\"UMask\": \"0x01\","
        "\"EventName\": \"TEST_EVENT2\","
        "\"Offcore\": \"1\","
        "\"other\": \"ignored\""
        "}]";
    EXPECT_NO_THROW(result = parse_perfmon(test_string));
    EXPECT_EQ(2u, result.size());
    PerfmonInfo event1 = result.at("TEST_EVENT");
    EXPECT_EQ(event1.m_event_name, "TEST_EVENT");
    EXPECT_EQ(event1.m_event_code.first, 0xD2);
    EXPECT_EQ(event1.m_umask, 0x04);
    EXPECT_EQ(event1.m_offcore, false);
    PerfmonInfo event2 = result.at("TEST_EVENT2");
    EXPECT_EQ(event2.m_event_name, "TEST_EVENT2");
    EXPECT_EQ(event2.m_event_code.first, 0xB7);
    EXPECT_EQ(event2.m_event_code.second, 0xBB);
    EXPECT_EQ(event2.m_umask, 0x01);
    EXPECT_EQ(event2.m_offcore, true);
}
