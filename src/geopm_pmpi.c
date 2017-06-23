/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>

#include "geopm.h"
#include "geopm_ctl.h"
#include "geopm_error.h"
#include "geopm_message.h"
#include "geopm_env.h"
#include "geopm_pmpi.h"
#include "geopm_sched.h"
#include "config.h"

static int g_is_geopm_pmpi_ctl_enabled = 0;
static int g_is_geopm_pmpi_prof_enabled = 0;
static MPI_Comm g_geopm_comm_world_swap = MPI_COMM_WORLD;
static MPI_Fint g_geopm_comm_world_swap_f = 0;
static MPI_Fint g_geopm_comm_world_f = 0;
static MPI_Comm g_ppn1_comm = MPI_COMM_NULL;
static struct geopm_ctl_c *g_ctl = NULL;
static pthread_t g_ctl_thread;

/* To be used only in Profile.cpp */
void geopm_pmpi_prof_enable(int do_profile)
{
    g_is_geopm_pmpi_prof_enabled = do_profile;
}

#ifndef GEOPM_PORTABLE_MPI_COMM_COMPARE_ENABLE
/*
 * Since MPI_COMM_WORLD should not be accessed or modified in this use
 * case, a simple == comparison will do and will be much more
 * performant than MPI_Comm_compare().
 */
static inline MPI_Comm geopm_swap_comm_world(MPI_Comm comm)
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
static MPI_Comm geopm_swap_comm_world(MPI_Comm comm)
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

void geopm_mpi_region_enter(uint64_t func_rid)
{
    if (g_is_geopm_pmpi_prof_enabled) {
        if (func_rid) {
            geopm_prof_enter(func_rid);
        }
        geopm_prof_enter(GEOPM_REGION_ID_MPI);
    }
}

void geopm_mpi_region_exit(uint64_t func_rid)
{
    if (g_is_geopm_pmpi_prof_enabled) {
        geopm_prof_exit(GEOPM_REGION_ID_MPI);
        if (func_rid) {
            geopm_prof_exit(func_rid);
        }
    }
}

uint64_t geopm_mpi_func_rid(const char *func_name)
{
    uint64_t result = 0;
    if (g_is_geopm_pmpi_prof_enabled) {
       int err = geopm_prof_region(func_name, GEOPM_REGION_HINT_NETWORK, &result);
       if (err) {
           result = 0;
       }
    }
    return result;
}

static int geopm_pmpi_init(const char *exec_name)
{
    int rank;
    int err = 0;
    PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
#ifdef GEOPM_DEBUG
    if (geopm_env_debug_attach() == rank) {
        char hostname[NAME_MAX];
        gethostname(hostname, sizeof(hostname));
        printf("PID %d on %s ready for attach\n", getpid(), hostname);
        fflush(stdout);
        volatile int cont = 0;
        while (!cont) {}
    }
#endif
    if (!err) {
        struct geopm_policy_c *policy = NULL;
        if (geopm_env_pmpi_ctl() == GEOPM_PMPI_CTL_PROCESS) {
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
                g_geopm_comm_world_f = PMPI_Comm_c2f(MPI_COMM_WORLD);
            }
            if (!err && is_ctl) {
                int ctl_rank;
                err = PMPI_Comm_rank(g_geopm_comm_world_swap, &ctl_rank);
                if (!err && !ctl_rank) {
                    err = geopm_policy_create(geopm_env_policy(), NULL, &policy);
                }
                if (!err) {
                    err = geopm_ctl_create(policy, g_geopm_comm_world_swap, &g_ctl);
                }
                if (!err) {
                    err = geopm_ctl_run(g_ctl);
                }
                int err_final = MPI_Finalize();
                err = err ? err : err_final;
                exit(err);
            }
        }
        else if (geopm_env_pmpi_ctl() == GEOPM_PMPI_CTL_PTHREAD) {
            g_is_geopm_pmpi_ctl_enabled = 1;

            int mpi_thread_level;
            pthread_attr_t thread_attr;
            int num_cpu = geopm_num_cpu();
            cpu_set_t *cpu_set = CPU_ALLOC(num_cpu);

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
                if (!err && !ppn1_rank) {
                    err = geopm_policy_create(geopm_env_policy(), NULL, &policy);
                }
                if (!err) {
                    err = geopm_ctl_create(policy, g_ppn1_comm, &g_ctl);
                }
                if (!err) {
                    err = pthread_attr_init(&thread_attr);
                }
                if (!err) {
                    err =  geopm_no_omp_cpu(num_cpu, cpu_set);
                }
                if (!err) {
                    err = pthread_attr_setaffinity_np(&thread_attr, 8 * num_cpu, cpu_set);
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
        if (!err && geopm_env_do_profile()) {
            geopm_prof_init();
        }
#ifdef GEOPM_DEBUG
        if (err) {
            char err_msg[NAME_MAX];
            geopm_error_message(err, err_msg, NAME_MAX);
            fprintf(stderr, "%s", err_msg);
        }
#endif
    }
    return err;
}

static int geopm_pmpi_finalize(void)
{
    int err = 0;
    int tmp_err = 0;

    if (!err && geopm_env_do_profile() &&
        (!g_ctl || geopm_env_pmpi_ctl() == GEOPM_PMPI_CTL_PTHREAD)) {
        err = geopm_prof_shutdown();
    }

    if (g_ctl && geopm_env_pmpi_ctl() == GEOPM_PMPI_CTL_PTHREAD) {
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

/* Below are the GEOPM PMPI wrappers */

int MPI_Init(int *argc, char **argv[])
{
    int err;

    if (geopm_env_pmpi_ctl() == GEOPM_PMPI_CTL_PTHREAD) {
        int required = MPI_THREAD_MULTIPLE;
        int mpi_thread_level;
        err = PMPI_Init_thread(argc, argv, required, &mpi_thread_level);
        if (!err && mpi_thread_level < MPI_THREAD_MULTIPLE) {
            err = GEOPM_ERROR_LOGIC;
        }
    }
    else {
        err = PMPI_Init(argc, argv);
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

int MPI_Init_thread(int *argc, char **argv[], int required, int *provided)
{
    int err = PMPI_Init_thread(argc, argv, required, provided);
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

int MPI_Finalize(void)
{
    int err = geopm_pmpi_finalize();
    int err_final = PMPI_Finalize();
    return err ? err : err_final;
}

/* Replace MPI_COMM_WORLD in all wrappers, but in the following
   blocking calls also profile with default profile */

int MPI_Allgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Allgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Allreduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Alltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Alltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Alltoallv(sendbuf, sendcounts,sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Alltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Barrier(MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Barrier(geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Bcast(buffer, count, datatype, root, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Bsend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Bsend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Bsend_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Bsend_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Gather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Gatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Neighbor_allgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Neighbor_allgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Neighbor_alltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Neighbor_alltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Neighbor_alltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST MPI_Aint sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST MPI_Aint rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}
#endif

int MPI_Reduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Reduce_scatter(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Reduce_scatter_block(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}
#endif

int MPI_Rsend(GEOPM_MPI_CONST void *ibuf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Rsend(ibuf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Rsend_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Rsend_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}



int MPI_Scan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Scan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Scatter(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Scatterv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

/* Profile non-blocking waits */
int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status *array_of_statuses)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Waitall(count, array_of_requests, array_of_statuses);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Waitany(int count, MPI_Request array_of_requests[], int *index, MPI_Status *status)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Waitany(count, array_of_requests, index, status);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Wait(request, status);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[])
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

/* Replace MPI_COMM_WORLD, but do not profile the rest of the APIs*/

#ifdef GEOPM_ENABLE_MPI3
int MPI_Iallgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Iallgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Iallreduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Ialltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Ialltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Ialltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm), request);
}

int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ibarrier(geopm_swap_comm_world(comm), request);
}

int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ibcast(buffer, count, datatype, root, geopm_swap_comm_world(comm), request);
}
#endif

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
    return PMPI_Cart_coords(geopm_swap_comm_world(comm), rank, maxdims, coords);
}

int MPI_Cart_create(MPI_Comm old_comm, int ndims, GEOPM_MPI_CONST int dims[], GEOPM_MPI_CONST int periods[], int reorder, MPI_Comm *comm_cart)
{
    return PMPI_Cart_create(geopm_swap_comm_world(old_comm), ndims, dims, periods, reorder, comm_cart);
}

int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[])
{
    return PMPI_Cart_get(geopm_swap_comm_world(comm), maxdims, dims, periods, coords);
}

int MPI_Cart_map(MPI_Comm comm, int ndims, GEOPM_MPI_CONST int dims[], GEOPM_MPI_CONST int periods[], int *newrank)
{
    return PMPI_Cart_map(geopm_swap_comm_world(comm), ndims, dims, periods, newrank);
}

int MPI_Cart_rank(MPI_Comm comm, GEOPM_MPI_CONST int coords[], int *rank)
{
    return PMPI_Cart_rank(geopm_swap_comm_world(comm), coords, rank);
}

int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest)
{
    return PMPI_Cart_shift(geopm_swap_comm_world(comm), direction, disp, rank_source, rank_dest);
}

int MPI_Cart_sub(MPI_Comm comm, GEOPM_MPI_CONST int remain_dims[], MPI_Comm *new_comm)
{
    return PMPI_Cart_sub(geopm_swap_comm_world(comm), remain_dims, new_comm);
}

int MPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
    return PMPI_Cartdim_get(geopm_swap_comm_world(comm), ndims);
}

int MPI_Comm_accept(GEOPM_MPI_CONST char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    return PMPI_Comm_accept(port_name, info, root, geopm_swap_comm_world(comm), newcomm);
}

// In mvapich this is defined as a macro.
#ifdef MPI_Comm_c2f
#undef MPI_Comm_c2f
#define MPI_Comm_c2f(comm) (MPI_Fint)(geopm_swap_comm_world(comm))
#else
MPI_Fint MPI_Comm_c2f(MPI_Comm comm)
{
    return PMPI_Comm_c2f(geopm_swap_comm_world(comm));
}
#endif

int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    return PMPI_Comm_call_errhandler(geopm_swap_comm_world(comm), errorcode);
}

int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    return PMPI_Comm_compare(geopm_swap_comm_world(comm1), geopm_swap_comm_world(comm2), result);
}

int MPI_Comm_connect(GEOPM_MPI_CONST char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    return PMPI_Comm_connect(port_name, info, root, geopm_swap_comm_world(comm), newcomm);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm)
{
    return PMPI_Comm_create_group(geopm_swap_comm_world(comm), group, tag, newcomm);
}
#endif

int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    return PMPI_Comm_create(geopm_swap_comm_world(comm), group, newcomm);
}

int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
    return PMPI_Comm_delete_attr(geopm_swap_comm_world(comm), comm_keyval);
}

int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    return PMPI_Comm_dup(geopm_swap_comm_world(comm), newcomm);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request)
{
    return PMPI_Comm_idup(geopm_swap_comm_world(comm), newcomm, request);
}

int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm)
{
    return PMPI_Comm_dup_with_info(geopm_swap_comm_world(comm), info, newcomm);
}
#endif

// In mvapich this is defined as a macro.
#ifdef MPI_Comm_f2c
#undef MPI_Comm_f2c
#define MPI_Comm_f2c(comm) geopm_swap_comm_world((MPI_Comm)(comm))
#else
MPI_Comm MPI_Comm_f2c(MPI_Fint comm)
{
    return geopm_swap_comm_world(PMPI_Comm_f2c(comm));
}
#endif

int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    return PMPI_Comm_get_attr(geopm_swap_comm_world(comm), comm_keyval, attribute_val, flag);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Dist_graph_create(MPI_Comm comm_old, int n, GEOPM_MPI_CONST int nodes[], GEOPM_MPI_CONST int degrees[], GEOPM_MPI_CONST int targets[], GEOPM_MPI_CONST int weights[], MPI_Info info, int reorder, MPI_Comm * newcomm)
{
    return PMPI_Dist_graph_create(geopm_swap_comm_world(comm_old), n, nodes, degrees, targets, weights, info, reorder, newcomm);
}

int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, GEOPM_MPI_CONST int sources[], GEOPM_MPI_CONST int sourceweights[], int outdegree, GEOPM_MPI_CONST int destinations[], GEOPM_MPI_CONST int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
{
    return PMPI_Dist_graph_create_adjacent(geopm_swap_comm_world(comm_old), indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph);
}

int MPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[])
{
    return PMPI_Dist_graph_neighbors(geopm_swap_comm_world(comm), maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights);
}

int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *inneighbors, int *outneighbors, int *weighted)
{
    return PMPI_Dist_graph_neighbors_count(geopm_swap_comm_world(comm), inneighbors, outneighbors, weighted);
}
#endif

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *erhandler)
{
    return PMPI_Comm_get_errhandler(geopm_swap_comm_world(comm), erhandler);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used)
{
    return PMPI_Comm_get_info(geopm_swap_comm_world(comm), info_used);
}
#endif

int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
    return PMPI_Comm_get_name(geopm_swap_comm_world(comm), comm_name, resultlen);
}

int MPI_Comm_get_parent(MPI_Comm *parent)
{
    return PMPI_Comm_get_parent(parent);
}

int MPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
    return PMPI_Comm_group(geopm_swap_comm_world(comm), group);
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
    return PMPI_Comm_rank(geopm_swap_comm_world(comm), rank);
}

int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
    return PMPI_Comm_remote_group(geopm_swap_comm_world(comm), group);
}

int MPI_Comm_remote_size(MPI_Comm comm, int *size)
{
    return PMPI_Comm_remote_size(geopm_swap_comm_world(comm), size);
}

int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
    return PMPI_Comm_set_attr(geopm_swap_comm_world(comm), comm_keyval, attribute_val);
}

int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
    return PMPI_Comm_set_errhandler(geopm_swap_comm_world(comm), errhandler);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    return PMPI_Comm_set_info(geopm_swap_comm_world(comm), info);
}
#endif

int MPI_Comm_set_name(MPI_Comm comm, GEOPM_MPI_CONST char *comm_name)
{
    return PMPI_Comm_set_name(geopm_swap_comm_world(comm), comm_name);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
    return PMPI_Comm_size(geopm_swap_comm_world(comm), size);
}

int MPI_Comm_spawn(GEOPM_MPI_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    return PMPI_Comm_spawn(command, argv, maxprocs, info, root, geopm_swap_comm_world(comm), intercomm, array_of_errcodes);
}

int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], GEOPM_MPI_CONST int array_of_maxprocs[], GEOPM_MPI_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    return PMPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, geopm_swap_comm_world(comm), intercomm, array_of_errcodes);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    return PMPI_Comm_split(geopm_swap_comm_world(comm), color, key, newcomm);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm)
{
    return PMPI_Comm_split_type(geopm_swap_comm_world(comm), split_type, key, info, newcomm);
}
#endif

int MPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
    return PMPI_Comm_test_inter(geopm_swap_comm_world(comm), flag);
}

int MPI_Exscan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Exscan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Iexscan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iexscan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm), request);
}
#endif

int MPI_File_open(MPI_Comm comm, GEOPM_MPI_CONST char *filename, int amode, MPI_Info info, MPI_File *fh)
{
    return PMPI_File_open(geopm_swap_comm_world(comm), filename, amode, info, fh);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Igather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Igatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, geopm_swap_comm_world(comm), request);
}
#endif

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, GEOPM_MPI_CONST int index[], GEOPM_MPI_CONST int edges[], int reorder, MPI_Comm *comm_graph)
{
    return PMPI_Graph_create(geopm_swap_comm_world(comm_old), nnodes, index, edges, reorder, comm_graph);
}

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[])
{
    return PMPI_Graph_get(geopm_swap_comm_world(comm), maxindex, maxedges, index, edges);
}

int MPI_Graph_map(MPI_Comm comm, int nnodes, GEOPM_MPI_CONST int index[], GEOPM_MPI_CONST int edges[], int *newrank)
{
    return PMPI_Graph_map(geopm_swap_comm_world(comm), nnodes, index, edges, newrank);
}

int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
    return PMPI_Graph_neighbors_count(geopm_swap_comm_world(comm), rank, nneighbors);
}

int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[])
{
    return PMPI_Graph_neighbors(geopm_swap_comm_world(comm), rank, maxneighbors, neighbors);
}

int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
    return PMPI_Graphdims_get(geopm_swap_comm_world(comm), nnodes, nedges);
}

int MPI_Ibsend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ibsend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status)
{
    return PMPI_Improbe(source, tag, geopm_swap_comm_world(comm), flag, message, status);
}
#endif

int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm bridge_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
{
    return PMPI_Intercomm_create(geopm_swap_comm_world(local_comm), local_leader, geopm_swap_comm_world(bridge_comm), remote_leader, tag, newintercomm);
}

int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintercomm)
{
    return PMPI_Intercomm_merge(geopm_swap_comm_world(intercomm), high, newintercomm);
}

int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
    return PMPI_Iprobe(source, tag, geopm_swap_comm_world(comm), flag, status);
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Irecv(buf, count, datatype, source, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Irsend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Irsend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Isend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Isend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Issend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Issend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status)
{
    return PMPI_Mprobe(source, tag, geopm_swap_comm_world(comm), message, status);
}

int MPI_Ineighbor_allgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Ineighbor_allgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Ineighbor_alltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Ineighbor_alltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Ineighbor_alltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST MPI_Aint sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST MPI_Aint rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm), request);
}
#endif

int MPI_Pack(GEOPM_MPI_CONST void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm)
{
    return PMPI_Pack(inbuf, incount, datatype, outbuf, outsize, position, geopm_swap_comm_world(comm));
}

int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
    return PMPI_Pack_size(incount, datatype, geopm_swap_comm_world(comm), size);
}

int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    return PMPI_Probe(source, tag, geopm_swap_comm_world(comm), status);
}

int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Recv_init(buf, count, datatype, source, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Recv(buf, count, datatype, source, tag, geopm_swap_comm_world(comm), status);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Ireduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, geopm_swap_comm_world(comm), request);
}

int MPI_Ireduce_scatter(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Ireduce_scatter_block(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Iscan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iscan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Iscatter(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Iscatterv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm), request);
}
#endif

int MPI_Send_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Send_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Send(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err =  PMPI_Send(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Sendrecv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, geopm_swap_comm_world(comm), status);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Sendrecv_replace(void * buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, geopm_swap_comm_world(comm), status);
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Ssend_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ssend_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Ssend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int err = 0;
    GEOPM_PMPI_ENTER_MACRO(__func__)
    err = PMPI_Ssend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
    GEOPM_PMPI_EXIT_MACRO
    return err;
}

int MPI_Topo_test(MPI_Comm comm, int *status)
{
    return PMPI_Topo_test(geopm_swap_comm_world(comm), status);
}

int MPI_Unpack(GEOPM_MPI_CONST void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
    return PMPI_Unpack(inbuf, insize, position, outbuf, outcount, datatype, geopm_swap_comm_world(comm));
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    return PMPI_Win_allocate(size, disp_unit, info, geopm_swap_comm_world(comm), baseptr, win);
}

int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    return PMPI_Win_allocate_shared(size, disp_unit, info, geopm_swap_comm_world(comm), baseptr, win);
}
#endif

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    return PMPI_Win_create(base, size, disp_unit, info, geopm_swap_comm_world(comm), win);
}

#ifdef GEOPM_ENABLE_MPI3
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    return PMPI_Win_create_dynamic(info, geopm_swap_comm_world(comm), win);
}
#endif
