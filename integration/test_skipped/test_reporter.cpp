/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm_reporter.h"
#include "geopm_pio.h"
#include "geopm_error.h"
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv)
{
    constexpr int report_max = 2 * 1024 * 1024;
    char report[report_max];

    int err = geopm_reporter_init();
    if (!err) {
        err = geopm_pio_read_batch();
    }
    if (!err) {
        err = geopm_reporter_update();
    }
    if (!err) {
        sleep(1);
        err = geopm_pio_read_batch();
    }
    if (!err) {
        err = geopm_reporter_update();
    }
    if (!err) {
        err = geopm_reporter_generate("profile_hello", "agent_hello", report_max, report);
    }
    if (!err) {
        std::cout << report;
    }
    else {
        geopm_error_message(err, report, report_max);
        std::cerr << report;
    }
    return err;
}
