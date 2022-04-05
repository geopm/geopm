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

#include <string>
#include <vector>
#include <unistd.h>

#include "geopm/Helper.hpp"
#include "geopm_hint.h"
#include "geopm_test.hpp"

TEST(HelperTest, string_split)
{
    std::vector<std::string> result;
    std::vector<std::string> expected;

    result = geopm::string_split("", " ");
    expected = {};
    EXPECT_EQ(expected, result);

    result = geopm::string_split(":", ":");
    expected = {"", ""};
    EXPECT_EQ(expected, result);

    result = geopm::string_split(" ", ":");
    expected = {" "};
    EXPECT_EQ(expected, result);

    result = geopm::string_split("one:two", " ");
    expected = {"one:two"};
    EXPECT_EQ(expected, result);

    result = geopm::string_split("one:two", ":");
    expected = {"one", "two"};
    EXPECT_EQ(expected, result);

    result = geopm::string_split(":one::two:three:", ":");
    expected = {"", "one", "", "two", "three", ""};
    EXPECT_EQ(expected, result);

    GEOPM_EXPECT_THROW_MESSAGE(geopm::string_split("one:two", ""),
                               GEOPM_ERROR_INVALID,
                               "invalid delimiter");
}

TEST(HelperTest, string_join)
{
    std::string result;
    result = geopm::string_join({}, ",");
    EXPECT_EQ("", result);

    result = geopm::string_join({"one"}, ":");
    EXPECT_EQ("one", result);

    result = geopm::string_join({"one", "two", "three"}, ", ");
    EXPECT_EQ("one, two, three", result);
}

TEST(HelperTest, string_begins_with)
{
    EXPECT_TRUE(geopm::string_begins_with("apple pie", "apple"));
    EXPECT_FALSE(geopm::string_begins_with("apple pie", "pie"));
    EXPECT_TRUE(geopm::string_begins_with("banana", "banana"));
    EXPECT_FALSE(geopm::string_begins_with("", "nothing"));
    EXPECT_TRUE(geopm::string_begins_with("nothing", ""));
}

TEST(HelperTest, string_ends_with)
{
    EXPECT_TRUE(geopm::string_ends_with("strawberry milkshake", "shake"));
    EXPECT_FALSE(geopm::string_ends_with("strawberry milkshake", "straw"));
    EXPECT_TRUE(geopm::string_ends_with("orange", "orange"));
    EXPECT_FALSE(geopm::string_ends_with("", "plum"));
    EXPECT_TRUE(geopm::string_ends_with("plum", ""));
}

TEST(HelperTest, check_hint)
{
    uint64_t hint = GEOPM_REGION_HINT_COMPUTE;
    EXPECT_NO_THROW(geopm::check_hint(hint));
    hint |= GEOPM_REGION_HINT_MEMORY;
    GEOPM_EXPECT_THROW_MESSAGE(geopm::check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "multiple region hints set");
    hint = 1ULL << 31;
    GEOPM_EXPECT_THROW_MESSAGE(geopm::check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "invalid hint");
}

TEST(HelperTest, pid_to)
{
    unsigned int uid = getuid();
    unsigned int gid = getgid();
    unsigned int pid = getpid();
    EXPECT_EQ(uid, geopm::pid_to_uid(pid));
    EXPECT_EQ(gid, geopm::pid_to_gid(pid));
}
