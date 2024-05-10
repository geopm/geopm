/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "geopm/Environment.hpp"
#include "geopm/Exception.hpp"
#include "geopm/ServiceProxy.hpp"
#include "geopm/Profile.hpp"
#include "geopm/PlatformIOProf.hpp"
#include "geopm_time.h"

static void __attribute__((constructor)) geopm_lib_init(void)
{
    if (geopm::environment().do_profile()) {
        try {
            geopm_time_s zero = geopm::time_zero();
            auto &prof = geopm::Profile::default_profile();
            // If this is a forked process, it will need to call
            // connect() since DefaultProfile constructor was called
            // by parent process.
            prof.connect();
            prof.overhead(geopm_time_since(&zero));
        }
        catch (...) {
            geopm::exception_handler(std::current_exception(), true);
        }
    }
}

