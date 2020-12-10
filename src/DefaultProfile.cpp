/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "Profile.hpp"

#include <unistd.h>
#include <string.h>

#include "geopm.h"
#include "ProfileThreadTable.hpp"
#include "Exception.hpp"

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
            err = geopm::exception_handler(std::current_exception());
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
                err = geopm::exception_handler(std::current_exception());
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
                err = geopm::exception_handler(std::current_exception());
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
                err = geopm::exception_handler(std::current_exception());
            }
        }
        else {
            err = GEOPM_ERROR_RUNTIME;
        }
        return err;
    }

    int geopm_prof_progress(uint64_t region_id, double fraction)
    {
        int err = 0;
        if (g_pmpi_prof_enabled) {
            try {
                geopm::Profile::default_profile().progress(region_id, fraction);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
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
                err = geopm::exception_handler(std::current_exception());
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
                err = geopm::exception_handler(std::current_exception());
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
        if (g_pmpi_tprof_enabled) {
            try {
                int cpu = geopm::Profile::get_cpu();
                geopm::Profile::default_profile().thread_init(cpu, num_work_unit);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
            }
        }
        return err;
    }

    int geopm_tprof_post(void)
    {
        int err = 0;
        if (g_pmpi_tprof_enabled) {
            try {
                int cpu = geopm::Profile::get_cpu();
                geopm::Profile::default_profile().thread_post(cpu);
            }
            catch (...) {
                err = geopm::exception_handler(std::current_exception());
            }
        }
        return err;
    }
}
