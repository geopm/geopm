/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "IOUringFallback.hpp"

#include <unistd.h>

#include "geopm/Helper.hpp"

#include <utility>

namespace geopm
{
    IOUringFallback::IOUringFallback(unsigned entries)
        : m_operations()
    {
        m_operations.reserve(entries);
    }

    IOUringFallback::~IOUringFallback()
    {
    }

    void IOUringFallback::submit()
    {
        errno = 0;
        for (const auto &operation : m_operations) {
            int ret = operation.second();
            if (ret < 0) {
                ret = -errno;
            }
            errno = 0;

            auto result_destination = operation.first;
            if (result_destination) {
                // The caller of prep_...() for this operation wants to
                // know the return value of the operation, so write it back.
                *result_destination = ret;
            }
        }

        m_operations.clear();
    }

    void IOUringFallback::prep_read(std::shared_ptr<int> ret, int fd, void *buf,
                                    unsigned nbytes, off_t offset)
    {
        m_operations.emplace_back(ret, std::bind(pread, fd, buf, nbytes, offset));
    }

    void IOUringFallback::prep_write(std::shared_ptr<int> ret, int fd, const void *buf,
                                     unsigned nbytes, off_t offset)
    {
        m_operations.emplace_back(ret, std::bind(pwrite, fd, buf, nbytes, offset));
    }

    std::unique_ptr<IOUring> IOUringFallback::make_unique(unsigned entries)
    {
        return geopm::make_unique<IOUringFallback>(entries);
    }
}
