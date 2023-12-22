/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "UniqueFd.hpp"

#include <unistd.h>

#include <iostream>
#include <cstring>

namespace geopm
{
UniqueFd::UniqueFd(int fd)
    : m_fd(fd)
{
}

// Move constructor: make sure the source object no longer closes the wrapped fd
UniqueFd::UniqueFd(UniqueFd &&other)
    : m_fd(other.m_fd)
{
    other.m_fd = -1;
}

// Close the wrapped fd if it is valid.
UniqueFd::~UniqueFd()
{
    if (m_fd >= 0) {
        int ret = close(m_fd);
        if (ret < 0) {
            std::cerr << "Warning: <geopm> UniqueFd encountered an error while closing file descriptor "
                      << std::to_string(m_fd) << ". Message: " << std::strerror(errno) << std::endl;
        }
    }
}

// Get the wrapped raw fd (e.g., to call IO functions on it)
int UniqueFd::get()
{
    return m_fd;
}
}
