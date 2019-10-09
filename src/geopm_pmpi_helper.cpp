/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#include "Environment.hpp"
#include "Exception.hpp"
#ifndef GEOPM_TEST
#include <mpi.h>
#endif
extern "C" {
#ifndef GEOPM_TEST
#include "geopm_ctl.h"
#endif
#include "geopm.h"
#include "geopm_error.h"
#include "geopm_internal.h"
#include "geopm_pmpi.h"
#include "geopm_sched.h"
#include "geopm_mpi_comm_split.h"
#include "config.h"
}


static int g_is_geopm_pmpi_ctl_enabled = 0;
static MPI_Comm g_geopm_comm_world_swap = MPI_COMM_WORLD;
static MPI_Fint g_geopm_comm_world_swap_f = 0;
static MPI_Fint g_geopm_comm_world_f = 0;
static MPI_Comm g_ppn1_comm = MPI_COMM_NULL;
static struct geopm_ctl_c *g_ctl = NULL;
#ifndef GEOPM_TEST
static pthread_t g_ctl_thread;
#endif

extern "C" {
    int geopm_is_pmpi_prof_enabled(void);

    static int geopm_env_pmpi_ctl(int *pmpi_ctl)
    {
        int err = 0;
        try {
            if (!pmpi_ctl) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *pmpi_ctl = geopm::environment().pmpi_ctl();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }

    static int geopm_env_do_profile(int *do_profile)
    {
        int err = 0;
        try {
            if (!do_profile) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *do_profile = geopm::environment().do_profile();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }

#ifdef GEOPM_DEBUG
    static int geopm_env_debug_attach(int *debug_attach)
    {
        int err = 0;
        try {
            if (!debug_attach) {
                err = GEOPM_ERROR_INVALID;
            }
            else {
                *debug_attach = geopm::environment().debug_attach();
            }
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), false);
        }
        return err;
    }
#endif

    void geopm_mpi_region_enter(uint64_t func_rid)
    {
        if (geopm_is_pmpi_prof_enabled()) {
            if (func_rid) {
                geopm_prof_enter(func_rid);
            }
            geopm_prof_enter(GEOPM_REGION_ID_MPI);
        }
    }

    void geopm_mpi_region_exit(uint64_t func_rid)
    {
        if (geopm_is_pmpi_prof_enabled()) {
            geopm_prof_exit(GEOPM_REGION_ID_MPI);
            if (func_rid) {
                geopm_prof_exit(func_rid);
            }
        }
    }

    uint64_t geopm_mpi_func_rid(const char *func_name)
    {
        uint64_t result = 0;
        if (geopm_is_pmpi_prof_enabled()) {
            int err = geopm_prof_region(func_name, GEOPM_REGION_HINT_NETWORK, &result);
            if (err) {
                result = 0;
            }
        }
        return result;
    }
}

#ifndef GEOPM_TEST
static int geopm_pmpi_init(const char *exec_name)
{
    int rank;
    int err = 0;
    int pmpi_ctl = 0;
    int do_profile = 0;
    g_geopm_comm_world_swap_f = PMPI_Comm_c2f(MPI_COMM_WORLD);
    g_geopm_comm_world_f = PMPI_Comm_c2f(MPI_COMM_WORLD);
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
#ifdef GEOPM_DEBUG
    int debug_attach = -1;
    err = geopm_env_debug_attach(&debug_attach);
    if (!err &&
        debug_attach == rank) {
        char hostname[NAME_MAX];
        gethostname(hostname, sizeof(hostname));
        printf("PID %d on %s ready for attach\n", getpid(), hostname);
        fflush(stdout);
        volatile int cont = 0;
        while (!cont) {}
    }
#endif
    if (!err) {
        err = geopm_env_pmpi_ctl(&pmpi_ctl);
        if (!err &&
            pmpi_ctl == GEOPM_CTL_PROCESS) {
            g_is_geopm_pmpi_ctl_enabled = 1;
            int is_ctl;
            MPI_Comm tmp_comm;
            err = geopm_comm_split(MPI_COMM_WORLD, "pmpi", &tmp_comm, &is_ctl);
            if (err) {
                MPI_Abort(MPI_COMM_WORLD, err);
            }
            else {
                g_geopm_comm_world_swap = tmp_comm;
                g_geopm_comm_world_swap_f = PMPI_Comm_c2f(g_geopm_comm_world_swap);
            }
            if (!err && is_ctl) {
                err = geopm_ctl_create(g_geopm_comm_world_swap, &g_ctl);
                if (!err) {
                    err = geopm_ctl_run(g_ctl);
                }
                int err_final = MPI_Finalize();
                err = err ? err : err_final;
                if (err) {
                    char err_msg[NAME_MAX];
                    geopm_error_message_last(err_msg, NAME_MAX);
                    if (strlen(err_msg) == 0) {
                        geopm_error_message(err, err_msg, NAME_MAX);
                    }
                    fprintf(stderr, "Error: %s\n", err_msg);
                }
                exit(err);
            }
        }
        else if (!err &&
                 pmpi_ctl == GEOPM_CTL_PTHREAD) {
            g_is_geopm_pmpi_ctl_enabled = 1;

            int mpi_thread_level = 0;
            pthread_attr_t thread_attr;
            int num_cpu = geopm_sched_num_cpu();
            cpu_set_t *cpu_set = CPU_ALLOC(num_cpu);
            if (NULL == cpu_set) {
                err = ENOMEM;
            }
            if (!err) {
                err = PMPI_Query_thread(&mpi_thread_level);
            }
            if (!err && mpi_thread_level < MPI_THREAD_MULTIPLE) {
                err = GEOPM_ERROR_LOGIC;
            }
            if (!err) {
                err = geopm_comm_split_ppn1(MPI_COMM_WORLD, "pmpi", &g_ppn1_comm);
            }
            if (!err && g_ppn1_comm != MPI_COMM_NULL) {
                int ppn1_rank;
                err = MPI_Comm_rank(g_ppn1_comm, &ppn1_rank);
                if (!err) {
                    err = geopm_ctl_create(g_ppn1_comm, &g_ctl);
                }
                if (!err) {
                    err = pthread_attr_init(&thread_attr);
                }
                if (!err) {
                    err = geopm_sched_woomp(num_cpu, cpu_set);
                }
                if (!err) {
                    err = pthread_attr_setaffinity_np(&thread_attr, CPU_ALLOC_SIZE(num_cpu), cpu_set);
                }
                if (!err) {
                    err = geopm_ctl_pthread(g_ctl, &thread_attr, &g_ctl_thread);
                }
                if (!err) {
                    err = pthread_attr_destroy(&thread_attr);
                }
            }
            CPU_FREE(cpu_set);
        }
        if (!err) {
            err = geopm_env_do_profile(&do_profile);
        }
        if (!err &&
            do_profile) {
            geopm_prof_init();
        }

        if (err) {
            char err_msg[NAME_MAX];
            geopm_error_message_last(err_msg, NAME_MAX);
            if (strlen(err_msg) == 0) {
                geopm_error_message(err, err_msg, NAME_MAX);
            }
            fprintf(stderr, "Error: %s\n", err_msg);
        }
    }
    return err;
}

extern "C" {
#ifndef GEOPM_PORTABLE_MPI_COMM_COMPARE_ENABLE
    /*
     * Since MPI_COMM_WORLD should not be accessed or modified in this use
     * case, a simple == comparison will do and will be much more
     * performant than MPI_Comm_compare().
     */
    MPI_Comm geopm_swap_comm_world(MPI_Comm comm)
    {
        return comm != MPI_COMM_WORLD ?
               comm : g_geopm_comm_world_swap;
    }
#else
    /*
     * The code below is more portable, but slower.  Define
     * GEOPM_ENABLE_PORTABLE_MPI_COMM_COMPARE if there are issues with the
     * direct comparison code above.
     */
    MPI_Comm geopm_swap_comm_world(MPI_Comm comm)
    {
        int is_comm_world = 0;
        (void)PMPI_Comm_compare(MPI_COMM_WORLD, comm, &is_comm_world);
        if (is_comm_world != MPI_UNEQUAL) {
            comm = g_geopm_comm_world_swap;
        }
        return comm;
    }
#endif

    MPI_Fint geopm_swap_comm_world_f(MPI_Fint comm)
    {
        return comm != g_geopm_comm_world_f ?
               comm : g_geopm_comm_world_swap_f;
    }

    int geopm_pmpi_init_thread(int *argc, char **argv[], int required, int *provided)
    {
        int err = 0;
        int pmpi_ctl = 0;

        err = geopm_env_pmpi_ctl(&pmpi_ctl);
        if (!err &&
            pmpi_ctl == GEOPM_CTL_PTHREAD &&
            required < MPI_THREAD_MULTIPLE) {
            required = MPI_THREAD_MULTIPLE;
        }
        err = PMPI_Init_thread(argc, argv, required, provided);
        if (!err &&
            pmpi_ctl == GEOPM_CTL_PTHREAD &&
            *provided < MPI_THREAD_MULTIPLE) {
            err = GEOPM_ERROR_RUNTIME;
        }
        if (!err) {
            err = PMPI_Barrier(MPI_COMM_WORLD);
        }
        if (!err) {
            if (argv && *argv && **argv && strlen(**argv)) {
                err = geopm_pmpi_init(**argv);
            }
            else {
                err = geopm_pmpi_init("Fortran");
            }
        }
        return err;
    }

    int geopm_pmpi_finalize(void)
    {
        int err = 0;
        int tmp_err = 0;
        int pmpi_ctl = 0;
        int do_profile = 0;

        err = geopm_env_pmpi_ctl(&pmpi_ctl);
        if (!err) {
            err = geopm_env_do_profile(&do_profile);
        }
        if (!err && do_profile &&
            (!g_ctl || pmpi_ctl == GEOPM_CTL_PTHREAD))
        {
            PMPI_Barrier(g_geopm_comm_world_swap);
            err = geopm_prof_shutdown();
        }

        if (!err && g_ctl && pmpi_ctl == GEOPM_CTL_PTHREAD) {
            void *return_val;
            err = pthread_join(g_ctl_thread, &return_val);
            err = err ? err : ((long)return_val);
        }

        if (!err && g_ctl) {
            err = geopm_ctl_destroy(g_ctl);
        }

        PMPI_Barrier(MPI_COMM_WORLD);

        if (g_geopm_comm_world_swap != MPI_COMM_WORLD) {
            tmp_err = PMPI_Comm_free(&g_geopm_comm_world_swap);
            err = err ? err : tmp_err;
        }
        if (g_ppn1_comm != MPI_COMM_NULL) {
            tmp_err = PMPI_Comm_free(&g_ppn1_comm);
            err = err ? err : tmp_err;
        }
        return err;
    }
}
#endif
