/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RUNTIMESERVICE_HPP_INCLUDE
#define RUNTIMESERVICE_HPP_INCLUDE

#include <string>

namespace geopm
{
    /// @brief The main entry point for the geopmrtd service daemon
    ///
    /// This command line tool supports the gRPC service described in
    /// the geopm_runtime.proto protobuffer description.
    ///
    /// @param [in] server_address The IP address and port where the
    ///        GEOPM Runtime gRPC service will be provided,
    ///        e.g. "123.100.0.1:8080".
    ///
    /// @return Zero on success, return code on failure.
    int rtd_main(const std::string &server_address);
}

#endif
