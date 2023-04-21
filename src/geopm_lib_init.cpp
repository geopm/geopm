/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "Environment.hpp"
#include "geopm/Exception.hpp"
#include "geopm/ServiceProxy.hpp"
#include "Profile.hpp"
#include "PlatformIOProf.hpp"
#include "geopm_time.h"

static void __attribute__((constructor)) geopm_lib_init(void)
{
    if (geopm::environment().do_profile()) {
        try {
            std::string profile_name = geopm::environment().profile();
            geopm::PlatformIOProf::platform_io();
            geopm_time_s zero;
            geopm_time(&zero);
            geopm::time_zero_reset(zero);
            geopm::Profile::default_profile();
        }
        catch (...) {
            geopm::exception_handler(std::current_exception(), true);
        }
    }
}

static void __attribute__((destructor)) geopm_lib_fini(void)
{
    if (geopm::environment().do_profile()) {
        try {
            geopm::Profile::default_profile().shutdown();
        }
        catch (...) {
            geopm::exception_handler(std::current_exception(), true);
        }
    }
}
