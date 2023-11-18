/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <unistd.h>
#include <string>
#include "geopm/Helper.hpp"


int main(int argc, char **argv)
{
    int pid = getpid();
    if (argc > 1) {
        pid = std::stoi(argv[1]);
    }
    if (geopm::has_cap_sys_admin(pid)) {
        return 0;
    }
    return -1;
}
