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

TEST(HelperTest, replace_all_substrings)
{
    std::string source_dest;

    source_dest = "nullanullabanull";
    geopm::replace_all_substrings(source_dest, "null", "nan");
    EXPECT_EQ("nanananabanan", source_dest);

    source_dest = "nullanullabanull";
    geopm::replace_all_substrings(source_dest, "null", "null");
    EXPECT_EQ("nullanullabanull", source_dest);

    source_dest = "nanananabanan";
    geopm::replace_all_substrings(source_dest, "nan", "null");
    EXPECT_EQ("nullanullabanull", source_dest);

    source_dest = "nullull";
    geopm::replace_all_substrings(source_dest, "null", "nan");
    EXPECT_EQ("nanull", source_dest);

    source_dest = "bananas";
    geopm::replace_all_substrings(source_dest, "banana", "fig");
    EXPECT_EQ("figs", source_dest);

    source_dest = "bananas";
    geopm::replace_all_substrings(source_dest, "nan", "null");
    EXPECT_EQ("banullas", source_dest);

    source_dest = "bananas";
    geopm::replace_all_substrings(source_dest, "pea", "bean");
    EXPECT_EQ("bananas", source_dest);

    source_dest = "asdfghjklasdfghjkl";
    geopm::replace_all_substrings(source_dest, "asdfghjkl", "asdf");
    EXPECT_EQ("asdfasdf", source_dest);

    source_dest = "bananas";
    geopm::replace_all_substrings(source_dest, "nan", "");
    EXPECT_EQ("baas", source_dest);

    source_dest = "nullnull";
    geopm::replace_all_substrings(source_dest, "null", "");
    EXPECT_EQ("", source_dest);

    source_dest = "nananan";
    geopm::replace_all_substrings(source_dest, "nan", "");
    EXPECT_EQ("a", source_dest);

    source_dest = "";
    geopm::replace_all_substrings(source_dest, "", "null");
    EXPECT_EQ("null", source_dest);

    source_dest = "";
    geopm::replace_all_substrings(source_dest, "nan", "null");
    EXPECT_EQ("", source_dest);

    source_dest = "";
    geopm::replace_all_substrings(source_dest, "", "");
    EXPECT_EQ("", source_dest);

    source_dest = "bananas";
    geopm::replace_all_substrings(source_dest, "", "null");
    EXPECT_EQ("bananas", source_dest);
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
