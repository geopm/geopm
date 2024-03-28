/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_time.h"


int run(int num_loop, double delay)
{
    auto &pio = geopm::platform_io();
    const auto &topo = geopm::platform_topo();
    std::vector<std::pair<std::string, int> > signal_list = {{"TIME", GEOPM_DOMAIN_BOARD},
                                                             {"CPU_FREQUENCY_STATUS", GEOPM_DOMAIN_CPU},
                                                             {"CPU_INSTRUCTIONS_RETIRED", GEOPM_DOMAIN_CPU},
                                                             {"CPU_CORE_TEMPERATURE", GEOPM_DOMAIN_CORE},
                                                             {"CPU_ENERGY", GEOPM_DOMAIN_PACKAGE},
                                                             {"DRAM_ENERGY", GEOPM_DOMAIN_PACKAGE}};

    std::vector<int> signal_idx;
    for (const auto &sig : signal_list) {
        for (int domain_idx = 0; domain_idx < topo.num_domain(sig.second); ++domain_idx) {
            signal_idx.push_back(pio.push_signal(sig.first, sig.second, domain_idx));
        }
    }
    double sum = 0;
    geopm_time_s time_0;
    geopm_time_s time_1;
    struct timespec delay_ts = {(time_t)delay,
                                (long)((delay - (time_t)delay) * 1e9)};
    std::vector<double> timings(num_loop, 0.0);
    int err = 0;
    for (int loop_idx = 0; loop_idx < num_loop; ++loop_idx) {
        geopm_time(&time_0);
        pio.read_batch();
        for (const auto &si : signal_idx) {
            sum += pio.sample(si);
        }
        geopm_time(&time_1);
        timings[loop_idx] = geopm_time_diff(&time_0, &time_1);
        do {
            err = clock_nanosleep(CLOCK_REALTIME, 0,
                                  &(delay_ts), nullptr);
        } while(err == EINTR);
    }
    std::cout << "COUNT,DURATION" << std::endl;
    for (const auto &tt : timings) {
        std::cout << signal_idx.size() << ","  << tt << std::endl;
    }
    return sum;
}


int main (int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << argv[0] << " LOOP_COUNT DELAY" << std::endl;
        return -1;
    }
    run(std::stoi(argv[1]), std::stod(argv[2]));
    return 0;
}
