/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "Environment.hpp"
#include "geopm_pio.h"

extern "C" {
    /**
     * Helper that creates the DefaultProfile signleton (if not already created)
     * and catches all exceptions.
     */
    int geopm_prof_init(void);
}

static void __attribute__((constructor)) geopm_lib_init(void)
{
    if (geopm::environment().do_profile()) {
        std::string profile_name = geopm::environment().profile();
        geopm_pio_start_profile(profile_name.c_str());
        geopm_prof_init();
    }
}

static void __attribute__((destructor)) geopm_lib_fini(void)
{
    if (geopm::environment().do_profile()) {
        geopm_pio_stop_profile();
    }
}
