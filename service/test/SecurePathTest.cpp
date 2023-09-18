/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <fstream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"
#include "SecurePath.hpp"

using geopm::SecurePath;
using testing::Throw;

class SecurePathTest : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        std::string m_file_name;
};

void SecurePathTest::SetUp()
{
    m_file_name = "SecurePathTest-regular_file";
    std::ofstream test_fd(m_file_name);
    test_fd << "This is a test of the emergency broadcast system." << std::endl;
    test_fd.close();
}

void SecurePathTest::TearDown()
{
    (void)!unlink(m_file_name.c_str());
}

TEST_F(SecurePathTest, umask)
{
    mode_t test_perms = (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); // 0o644
    chmod(m_file_name.c_str(), test_perms);
    std::string fd_path = "";

    // Assert write permissions for the group/world are not set
    {
        SecurePath sp (m_file_name, (S_IWGRP | S_IWOTH), true);
        EXPECT_TRUE(geopm::string_begins_with(sp.secure_path(), "/proc/self/fd"));
        fd_path = sp.secure_path();
    }

    // When not enforcing, umask is ignored
    {
        SecurePath sp (m_file_name, (S_IWUSR), false);
        EXPECT_EQ(fd_path, sp.secure_path());
    }

    // When enforcing, an exception is generated
    GEOPM_EXPECT_THROW_MESSAGE(SecurePath(m_file_name, (S_IWUSR), true),
                               GEOPM_ERROR_RUNTIME, "File has invalid permissions");
}

TEST_F(SecurePathTest, bad_file)
{
    std::string bad_file = m_file_name + "-link";
    (void)!symlink(m_file_name.c_str(), bad_file.c_str());

    GEOPM_EXPECT_THROW_MESSAGE(SecurePath(bad_file, (S_IWGRP | S_IWOTH), true),
                               ELOOP, "Failed to open file");
    (void)!unlink(bad_file.c_str());

    std::string bad_file_2 = m_file_name + "-dir";
    mode_t default_mode = (S_IRWXU);
    (void)!mkdir(bad_file_2.c_str(), default_mode);

    GEOPM_EXPECT_THROW_MESSAGE(SecurePath(bad_file_2, (S_IWGRP | S_IWOTH), true),
                               GEOPM_ERROR_RUNTIME, "File not a regular file");

    GEOPM_EXPECT_THROW_MESSAGE(SecurePath("/dev/null", (S_IWGRP | S_IWOTH), true),
                               GEOPM_ERROR_RUNTIME, "File not owned by current user");

    GEOPM_EXPECT_THROW_MESSAGE(SecurePath("/etc/shadow", (S_IWGRP | S_IWOTH), true),
                               EACCES, "Failed to open file");
    (void)!rmdir(bad_file_2.c_str());
}
