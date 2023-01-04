/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKIOURING_HPP_INCLUDE
#define MOCKIOURING_HPP_INCLUDE

#include "IOUring.hpp"

#include "gmock/gmock.h"

class MockIOUring : public geopm::IOUring
{
    public:
        MOCK_METHOD(void, submit, (), (override));
        MOCK_METHOD(void, prep_read,
                    (std::shared_ptr<int> ret, int fd,
                     void *buf, unsigned nbytes, off_t offset),
                    (override));
        MOCK_METHOD(void, prep_write,
                    (std::shared_ptr<int> ret, int fd,
                     const void *buf, unsigned nbytes, off_t offset),
                    (override));
};

#endif /* MOCKIOURING_HPP_INCLUDE */
