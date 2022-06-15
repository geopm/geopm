/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
    uint64_t hint = GEOPM_SENTINEL_REGION_HINT << 32;
    GEOPM_EXPECT_THROW_MESSAGE(geopm::check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "hint out of range");
    hint = 1ULL << 31;
    GEOPM_EXPECT_THROW_MESSAGE(geopm::check_hint(hint),
                               GEOPM_ERROR_INVALID,
                               "invalid hint");
}

TEST(HelperTest, hint_to_index)
{
    uint64_t h;
    uint64_t i;
    for (h = (1ULL << 32), i = 1; h < (GEOPM_SENTINEL_REGION_HINT << 32);
         h += (1ULL << 32), i++) {
        EXPECT_EQ(i, geopm::hint_to_index(h));
    }

    // "UNSET" is "index" 0
    EXPECT_EQ(0, geopm::hint_to_index(GEOPM_REGION_HINT_UNSET));
    uint64_t index_out_of_bounds = GEOPM_SENTINEL_REGION_HINT + 1;
    // hint_to_index() does not perform bounds checking
    EXPECT_EQ(index_out_of_bounds,
              geopm::hint_to_index(index_out_of_bounds << 32));
}

TEST(HelperTest, pid_to)
{
    unsigned int uid = getuid();
    unsigned int gid = getgid();
    unsigned int pid = getpid();
    EXPECT_EQ(uid, geopm::pid_to_uid(pid));
    EXPECT_EQ(gid, geopm::pid_to_gid(pid));
}
