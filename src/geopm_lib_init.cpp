/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "Environment.hpp"
#include "geopm/Exception.hpp"
#include "geopm/ServiceProxy.hpp"
#include "Profile.hpp"
#include "PlatformIOProf.hpp"

static void __attribute__((constructor)) geopm_lib_init(void)
{
    if (geopm::environment().do_profile()) {
        try {
            std::string profile_name = geopm::environment().profile();
            auto service_proxy = geopm::ServiceProxy::make_unique();
            service_proxy->platform_start_profile(profile_name);
            geopm::Profile::default_profile();
            geopm::PlatformIOProf::platform_io();
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
            auto region_names = geopm::Profile::default_profile().region_names();
            auto service_proxy = geopm::ServiceProxy::make_unique();
            service_proxy->platform_stop_profile(region_names);
        }
        catch (...) {
            geopm::exception_handler(std::current_exception(), true);
        }
    }
}
