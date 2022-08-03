/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include <string>
#include <iostream>
#include <sstream>

#include "Admin.hpp"
#include "geopm/Exception.hpp"

int main(int argc, const char **argv)
{
    int err = 0;
    try {
        geopm::Admin admin;
        admin.main(argc, argv, std::cout, std::cerr);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception(), true);
    }
    return err;
}
