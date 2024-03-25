/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "IOUring.hpp"

#include <errno.h>
#include <fcntl.h>

#include "IOUringFallback.hpp"
#include "geopm_test.hpp"

#include "gtest/gtest.h"

#include <memory>

using geopm::IOUring;

class IOUringTest : public ::testing::TestWithParam<std::shared_ptr<IOUring> >
{
    protected:
        void test_reads(const std::string &context, std::shared_ptr<IOUring> io);
        void test_writes(const std::string &context, std::shared_ptr<IOUring> io);
};

void IOUringTest::test_reads(const std::string &context, std::shared_ptr<IOUring> io)
{
    auto write_only_errno = std::make_shared<int>(12345);
    int write_only_fd = open("/dev/zero", O_WRONLY);
    ASSERT_GT(write_only_fd, -1) << context << ": Failed to open /dev/zero for writing";
    int unusable_buf = 10;
    io->prep_read(write_only_errno,
                  write_only_fd, &unusable_buf, sizeof unusable_buf, 0);

    auto read_only_errno = std::make_shared<int>(12345);
    int read_only_fd = open("/dev/zero", O_RDONLY);
    ASSERT_GT(read_only_fd, -1) << context << ": Failed to open /dev/zero for reading";
    int dev_zero_buf = 10;
    io->prep_read(read_only_errno,
                  read_only_fd, &dev_zero_buf, sizeof dev_zero_buf, 0);

    io->submit();

    EXPECT_EQ(-EBADF, *write_only_errno) << context;
    EXPECT_EQ(static_cast<int>(sizeof dev_zero_buf), *read_only_errno) << context;
    EXPECT_EQ(0, dev_zero_buf) << context;
}

void IOUringTest::test_writes(const std::string &context, std::shared_ptr<IOUring> io)
{
    auto write_only_errno = std::make_shared<int>(12345);
    int write_only_fd = open("/dev/null", O_WRONLY);
    ASSERT_GT(write_only_fd, -1) << context << ": Failed to open /dev/null for writing";
    int unusable_buf = 10;
    io->prep_write(write_only_errno,
                   write_only_fd, &unusable_buf, sizeof unusable_buf, 0);

    auto read_only_errno = std::make_shared<int>(12345);
    int read_only_fd = open("/dev/null", O_RDONLY);
    ASSERT_GT(read_only_fd, -1) << context << ": Failed to open /dev/null for reading";
    int dev_null_buf = 10;
    io->prep_write(read_only_errno,
                   read_only_fd, &dev_null_buf, sizeof dev_null_buf, 0);

    io->submit();

    EXPECT_EQ(static_cast<int>(sizeof dev_null_buf), *write_only_errno) << context;
    EXPECT_EQ(-EBADF, *read_only_errno) << context;
}

TEST_F(IOUringTest, batch_read)
{
    // If GEOPM is built without IO uring, these are both the same test.
    test_reads("uring", geopm::IOUring::make_unique(2));
    test_reads("fallback", geopm::IOUringFallback::make_unique(2));
}

TEST_F(IOUringTest, batch_write)
{
    // If GEOPM is built without IO uring, these are both the same test.
    test_writes("uring", geopm::IOUring::make_unique(2));
    test_writes("fallback", geopm::IOUringFallback::make_unique(2));
}
