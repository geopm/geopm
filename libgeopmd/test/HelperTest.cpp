/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"

#include <fcntl.h>
#include <string>
#include <vector>
#include <unistd.h>

#include "geopm/Helper.hpp"
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

TEST(HelperTest, pid_to)
{
    unsigned int uid = getuid();
    unsigned int gid = getgid();
    unsigned int pid = getpid();
    EXPECT_EQ(uid, geopm::pid_to_uid(pid));
    EXPECT_EQ(gid, geopm::pid_to_gid(pid));
}


TEST(HelperTest, has_cap_sys_admin)
{
    GEOPM_TEST_EXTENDED("Capabilities requirements");
    if (getuid() != 0) {
        EXPECT_FALSE(geopm::has_cap_sys_admin());
        EXPECT_FALSE(geopm::has_cap_sys_admin(getpid()));
    }
    else {
        std::cerr << "Warning: running unit tests as \"root\" user is not advised\n";
        EXPECT_TRUE(geopm::has_cap_sys_admin());
        EXPECT_TRUE(geopm::has_cap_sys_admin(getpid()));
    }
}

TEST(HelperTest, read_symlink_target)
{
    static const std::string SYMLINK_PATH = "/tmp/HelperTest_read_symlink_target_" +
                                            std::to_string(getpid());
    int err = symlink("/some/made/up/path", SYMLINK_PATH.c_str());
    ASSERT_EQ(0, err) << "Unable to create symlink at " << SYMLINK_PATH;
    EXPECT_NO_THROW({
        EXPECT_EQ("/some/made/up/path", geopm::read_symlink_target(SYMLINK_PATH));
    });
    err = unlink(SYMLINK_PATH.c_str());
    ASSERT_NE(-1, err) << "Unable to remove symlink at " << SYMLINK_PATH;

    err = open(SYMLINK_PATH.c_str(), O_RDWR|O_CREAT, 0644);
    ASSERT_NE(-1, err) << "Unable to create non-symlink file at " << SYMLINK_PATH
                       << " Reason: " << strerror(errno);
    EXPECT_THROW(geopm::read_symlink_target(SYMLINK_PATH), geopm::Exception)
        << "Expect an exception when reading a symlink target of a non-symlink";
    err = unlink(SYMLINK_PATH.c_str());
    ASSERT_NE(-1, err) << "Unable to remove non-symlink file at " << SYMLINK_PATH;

    EXPECT_THROW(geopm::read_symlink_target(SYMLINK_PATH), geopm::Exception)
        << "Expect an exception when reading an absent symlink";
}
