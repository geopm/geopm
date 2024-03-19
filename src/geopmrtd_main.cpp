/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "RuntimeService.hpp"

#include <sstream>
#include <iostream>

#include "geopm_version.h"

/// @brief Entry point for geopmrtd daemon command line tool
int main(int argc, char **argv)
{
    std::ostringstream usage;
    usage << "Usage: " << argv[0] << " SERVER_ADDRESS\n\n";
    if (argc != 2) {
        std::cerr << usage.str();
        return -1;
    }
    std::string server_address(argv[1]);
    if (server_address == "--help" ||
        server_address == "-h") {
        std::cout << usage.str();
        return 0;
    }
    if (server_address == "--version") {
        std::cout << geopm_version() << "\n\n"
                  << "Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.\n\n";
        return 0;
    }
    return geopm::rtd_main(server_address);
}
