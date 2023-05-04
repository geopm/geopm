/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include "Profile.hpp"

#include <unistd.h>
#include <string.h>

#include "geopm_prof.h"
#include "geopm/Exception.hpp"

// Don't allow multithreaded use of non-tprof profile calls
static __thread int g_prof_enabled = 0;

namespace geopm
{
    class DefaultProfile : public ProfileImp
    {
        public:
            DefaultProfile();
            virtual ~DefaultProfile() = default;
    };

    DefaultProfile::DefaultProfile()
        : ProfileImp()
    {
        g_prof_enabled = 1;
    }


    Profile &Profile::default_profile(void)
    {
        static geopm::DefaultProfile instance;
        return instance;
    }
}


extern "C"
{
    int geopm_prof_region(const char *region_name, uint64_t hint, uint64_t *region_id)
    {
        int err = 0;
        if (g_prof_enabled) {
            try {
                *region_id = geopm::Profile::default_profile().region(std::string(region_name), hint);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_prof_enter(uint64_t region_id)
    {
        int err = 0;
        if (g_prof_enabled) {
            try {
                geopm::Profile::default_profile().enter(region_id);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_prof_exit(uint64_t region_id)
    {
        int err = 0;
        if (g_prof_enabled) {
            try {
                geopm::Profile::default_profile().exit(region_id);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_prof_epoch(void)
    {
        int err = 0;
        if (g_prof_enabled) {
            try {
                geopm::Profile::default_profile().epoch();
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_prof_shutdown(void)
    {
        int err = 0;
        if (g_prof_enabled) {
            try {
                geopm::Profile::default_profile().shutdown();
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_tprof_init(uint32_t num_work_unit)
    {
        int err = 0;
        // Only the lead thread calls through to thread_init()
        if (g_prof_enabled) {
            try {
                geopm::Profile::default_profile().thread_init(num_work_unit);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        return err;
    }

    int geopm_tprof_post(void)
    {
        int err = 0;
        // All threads call through to thread_post()
        try {
            int cpu = geopm::Profile::get_cpu();
            geopm::Profile::default_profile().thread_post(cpu);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_prof_overhead(double overhead_sec)
    {
        int err = 0;
        if (g_prof_enabled) {
            try {
                geopm::Profile::default_profile().overhead(overhead_sec);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

}
