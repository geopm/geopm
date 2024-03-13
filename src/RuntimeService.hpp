/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RUNTIMESERVICE_HPP_INCLUDE
#define RUNTIMESERVICE_HPP_INCLUDE

#include <string>

namespace geopm
{
    int rtd_main(const std::string &server_address);
}

#endif
