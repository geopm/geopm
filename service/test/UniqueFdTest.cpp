/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "UniqueFd.hpp"

#include <fcntl.h>

#include "gtest/gtest.h"

#include <vector>

using geopm::UniqueFd;

TEST(UniqueFdTest, closes_when_out_of_scope)
{
    int raw_fd = open("/dev/null", O_RDONLY);
    {
        UniqueFd scoped_fd = raw_fd;
        EXPECT_EQ(raw_fd, scoped_fd.get());
    }

    EXPECT_EQ(-1, fcntl(raw_fd, F_GETFD));
    EXPECT_EQ(EBADF, errno);
}

TEST(UniqueFdTest, transfers_close_responsibility_on_move)
{
    int raw_fd = open("/dev/null", O_RDONLY);
    std::vector<UniqueFd> fds;
    {
        UniqueFd scoped_fd = raw_fd;
        fds.push_back(std::move(scoped_fd));
    }
    // scoped_fd went out of scope, but fds[0] is now the owner, so the fd
    // should still be valid
    EXPECT_EQ(0, fcntl(raw_fd, F_GETFD));
    EXPECT_EQ(raw_fd, fds.back().get());

    // The new owner is now removed from the vector, so the fd should be invalid.
    fds.pop_back();
    EXPECT_EQ(-1, fcntl(raw_fd, F_GETFD));
    EXPECT_EQ(EBADF, errno);
}
