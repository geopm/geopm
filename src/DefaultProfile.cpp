/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "Profile.hpp"

#include <unistd.h>
#include <string.h>

#include "geopm_prof.h"
#include "geopm/Exception.hpp"

#include "config.h"

// Don't allow multithreaded use of non-tprof profile calls
static __thread int g_pmpi_prof_enabled = 0;
static int g_pmpi_tprof_enabled = 0;

namespace geopm
{
    class DefaultProfile : public ProfileImp
    {
        public:
            DefaultProfile();
            virtual ~DefaultProfile();
            void enable_pmpi(void) override;
    };

    DefaultProfile::DefaultProfile()
        : ProfileImp()
    {
        g_pmpi_prof_enabled = m_is_enabled;
        g_pmpi_tprof_enabled = m_is_enabled;
    }

    DefaultProfile::~DefaultProfile()
    {
        g_pmpi_prof_enabled = 0;
        g_pmpi_tprof_enabled = 0;
    }

    void DefaultProfile::enable_pmpi(void)
    {
        g_pmpi_prof_enabled = m_is_enabled;
        g_pmpi_tprof_enabled = m_is_enabled;
    }

    Profile &Profile::default_profile(void)
    {
        static geopm::DefaultProfile instance;
        return instance;
    }
}


extern "C"
{
    int geopm_prof_init(void)
    {
        int err = 0;
        try {
            geopm::Profile::default_profile().init();
            geopm::Profile::default_profile().enable_pmpi();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_prof_region(const char *region_name, uint64_t hint, uint64_t *region_id)
    {
        int err = 0;
        if (g_pmpi_prof_enabled) {
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
        if (g_pmpi_prof_enabled) {
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
        if (g_pmpi_prof_enabled) {
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
        if (g_pmpi_prof_enabled) {
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
        if (g_pmpi_prof_enabled) {
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
        if (g_pmpi_prof_enabled) {
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
        if (g_pmpi_tprof_enabled) {
            try {
                int cpu = geopm::Profile::get_cpu();
                geopm::Profile::default_profile().thread_post(cpu);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception(), true);
            }
        }
        return err;
    }
}
