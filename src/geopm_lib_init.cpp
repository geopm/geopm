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
            geopm_time_s zero = geopm::time_zero();
            auto &prof = geopm::Profile::default_profile();
            prof.overhead(geopm_time_since(&zero));
        }
        catch (...) {
            geopm::exception_handler(std::current_exception(), true);
        }
    }
}

