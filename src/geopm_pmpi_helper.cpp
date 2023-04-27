/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

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
#include "geopm/Exception.hpp"
#include "geopm/ServiceProxy.hpp"
#include "Profile.hpp"
#ifndef GEOPM_TEST
#include <mpi.h>
#endif

extern "C"
{
#ifndef GEOPM_TEST
#include "geopm_ctl.h"
#endif
#include "geopm_prof.h"
#include "geopm_error.h"
#include "geopm_hint.h"
#include "geopm_pmpi.h"
#include "geopm_sched.h"
#include "geopm_mpi_comm_split.h"
}

using geopm::Exception;

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

    void geopm_mpi_region_enter(uint64_t func_rid)
    {
        if (func_rid) {
            geopm_prof_enter(func_rid);
        }
    }

    void geopm_mpi_region_exit(uint64_t func_rid)
    {
        if (func_rid) {
            geopm_prof_exit(func_rid);
        }
    }

    uint64_t geopm_mpi_func_rid(const char *func_name)
    {
        uint64_t result = 0;
        int err = geopm_prof_region(func_name, GEOPM_REGION_HINT_NETWORK, &result);
        if (err) {
            result = 0;
        }
        return result;
    }
}

#ifndef GEOPM_TEST
static int geopm_pmpi_init(const char *exec_name)
{
    int do_profile = 0;
    int rank;
    int err = 0;
    int pmpi_ctl = 0;
    g_geopm_comm_world_swap_f = PMPI_Comm_c2f(MPI_COMM_WORLD);
    g_geopm_comm_world_f = PMPI_Comm_c2f(MPI_COMM_WORLD);
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
#ifdef GEOPM_DEBUG
    if (geopm::environment().do_debug_attach_all() ||
        (geopm::environment().do_debug_attach_one() &&
         geopm::environment().debug_attach_process() == rank)) {
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
            pmpi_ctl == geopm::Environment::M_CTL_PROCESS) {
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
                try {
                    geopm::Profile::default_profile().shutdown();
                }
                catch (const Exception &ex) {
                    throw Exception(std::string("Requested GEOPM Controller be launched in process mode,"
                                    " but GEOPM Service is not active: ") + ex.what(),
                                    GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
                }
                err = geopm_ctl_create(g_geopm_comm_world_swap, &g_ctl);
                if (!err) {
                    err = geopm_ctl_run(g_ctl);
                }
                int err_final = MPI_Finalize();
                err = err ? err : err_final;
                exit(err);
            }
        }
        else if (!err &&
                 pmpi_ctl == geopm::Environment::M_CTL_PTHREAD) {
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
            if (cpu_set) {
               CPU_FREE(cpu_set);
            }
        }
        if (!err) {
            err = geopm_env_do_profile(&do_profile);
        }
#ifdef GEOPM_DEBUG
        if (err) {
            char err_msg[PATH_MAX];
            geopm_error_message(err, err_msg, PATH_MAX);
            fprintf(stderr, "%s", err_msg);
        }
#endif
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

        geopm::Profile::default_profile().reset_cpu_set();
        uint64_t init_rid = geopm_mpi_func_rid("MPI_Init");
        geopm_mpi_region_enter(init_rid);

        int pmpi_ctl = 0;

        err = geopm_env_pmpi_ctl(&pmpi_ctl);
        if (!err &&
            pmpi_ctl == geopm::Environment::M_CTL_PTHREAD &&
            required < MPI_THREAD_MULTIPLE) {
            required = MPI_THREAD_MULTIPLE;
        }
        err = PMPI_Init_thread(argc, argv, required, provided);
        if (!err &&
            pmpi_ctl == geopm::Environment::M_CTL_PTHREAD &&
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
        if (!err) {
            geopm_mpi_region_exit(init_rid);
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
            (!g_ctl || pmpi_ctl == geopm::Environment::M_CTL_PTHREAD))
        {
            PMPI_Barrier(g_geopm_comm_world_swap);
            err = geopm_prof_shutdown();
        }

        if (!err && g_ctl && pmpi_ctl == geopm::Environment::M_CTL_PTHREAD) {
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
