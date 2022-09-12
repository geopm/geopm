/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
/// Main() built with fast FP math.

#undef NDEBUG

#include <cmath>
#include <cassert>
#include <iostream>
#include "geopm/PlatformIO.hpp"
#include "geopm_pio.h"
#include "geopm_topo.h"

bool check_valid_cpp(double value) {
    return geopm::PlatformIO::is_valid_value(value);
}

bool check_valid_c(double value) {
    return (0 == geopm_pio_check_valid_value(value));
}

int main(int argc, char **argv)
{
    double nan_value;
    int sample_idx = geopm_pio_push_signal("CPU_POWER", GEOPM_DOMAIN_BOARD, 0);
    if (sample_idx < 0) {
        std::cerr << "Error: push_signal() returned " << sample_idx << std::endl;
    }
    geopm_pio_read_batch();
    geopm_pio_sample(sample_idx, &nan_value);

    double non_nan_value = 0.0;
    geopm_pio_read_batch();
    geopm_pio_sample(sample_idx, &non_nan_value);

    assert(check_valid_cpp(nan_value) == false);
    assert(check_valid_c(nan_value) == false);

    assert(check_valid_cpp(non_nan_value) == true);
    assert(check_valid_c(non_nan_value) == true);

    std::cout << "All asserts have checked" << std::endl;
    std::cout << "The test_invalid_values has PASSED" << std::endl;

    return 0;
}
