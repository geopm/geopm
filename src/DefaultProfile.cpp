/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __APPLE__
#define _DARWIN_C_SOURCE
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <algorithm>
#include <iostream>
#include <sstream>

#include <float.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "geopm.h"
#include "geopm_message.h"
#include "geopm_time.h"
#include "geopm_signal_handler.h"
#include "geopm_sched.h"
#include "geopm_env.h"
#include "Profile.hpp"
#include "ProfileTable.hpp"
#include "ProfileThread.hpp"
#include "SampleScheduler.hpp"
#include "ControlMessage.hpp"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "Comm.hpp"

#include "config.h"

extern "C"
{
    // defined in geopm_pmpi.c and used only here
    void geopm_pmpi_prof_enable(int do_profile);
}

namespace geopm
{
    class DefaultProfile : public Profile {
        public:
            DefaultProfile(const std::string prof_name, const IComm *comm);
            virtual ~DefaultProfile();
    };

    DefaultProfile::DefaultProfile(const std::string prof_name, const IComm *comm)
        : Profile(prof_name, comm)
    {
        geopm_pmpi_prof_enable(1);
    }

    DefaultProfile::~DefaultProfile()
    {
        geopm_pmpi_prof_enable(0);
    }
}

static geopm::DefaultProfile &geopm_default_prof(void)
{
    static geopm::DefaultProfile default_prof(geopm_env_profile(),
            geopm::CommFactory::comm_factory().get_comm(geopm_env_comm()));
    return default_prof;
}

extern "C"
{
    int geopm_prof_init(void)
    {
        int err = 0;
        try {
            geopm_default_prof();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_region(const char *region_name, uint64_t hint, uint64_t *region_id)
    {
        int err = 0;
        try {
            *region_id = geopm_default_prof().region(std::string(region_name), hint);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_enter(uint64_t region_id)
    {
        int err = 0;
        try {
            geopm_default_prof().enter(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_exit(uint64_t region_id)
    {
        int err = 0;
        try {
            geopm_default_prof().exit(region_id);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_progress(uint64_t region_id, double fraction)
    {
        int err = 0;
        try {
            geopm_default_prof().progress(region_id, fraction);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_epoch(void)
    {
        int err = 0;
        try {
            geopm_default_prof().epoch();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_prof_shutdown(void)
    {
        int err = 0;
        try {
            geopm_default_prof().shutdown();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_init(uint32_t num_work_unit)
    {
        int err = 0;
        try {
            geopm_default_prof().tprof_table()->init(num_work_unit);
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_init_loop(int num_thread, int thread_idx, size_t num_iter, size_t chunk_size)
    {
        int err = 0;
        try {
            std::shared_ptr<geopm::IProfileThreadTable> table_ptr = geopm_default_prof().tprof_table();
            if (chunk_size) {
                table_ptr->init(num_thread, thread_idx, num_iter, chunk_size);
            }
            else {
                table_ptr->init(num_thread, thread_idx, num_iter);
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }

    int geopm_tprof_post(void)
    {
        int err = 0;
        try {
            geopm_default_prof().tprof_table()->post();
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception());
        }
        return err;
    }
}

