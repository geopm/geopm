/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <iostream>
#include <unistd.h>
#include "geopm/PlatformIO.hpp"
#include "geopm_topo.h"

void run(void)
{
    geopm::PlatformIO &pio = geopm::platform_io();
    int signal_idx = pio.push_signal("SERVICE::TIME", GEOPM_DOMAIN_CPU, 0);
    for (int idx = 0; idx < 10; ++idx) {
        pio.read_batch();
        double tt = pio.sample(signal_idx);
        std::cerr << tt << "\n";
        sleep(1);
    }
}


int main (int argc, char **argv)
{
    run();
    return 0;
}
