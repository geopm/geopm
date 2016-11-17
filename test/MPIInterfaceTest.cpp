/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <mpi.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include "geopm_env.h"

extern "C"
{
    static uint64_t g_test_curr_region_enter_id = 0;
    static uint64_t g_test_curr_region_enter_count = 0;
    int mock_geopm_prof_enter(uint64_t region_id)
    {
        g_test_curr_region_enter_id = region_id;
        g_test_curr_region_enter_count++;
        return 0;
    }
    #define geopm_prof_enter(a) mock_geopm_prof_enter(a)

    static uint64_t g_test_curr_region_exit_id = 0;
    static uint64_t g_test_curr_region_exit_count = 0;
    int mock_geopm_prof_exit(uint64_t region_id)
    {
        g_test_curr_region_exit_id = region_id;
        g_test_curr_region_exit_count++;
        return 0;
    }
    #define geopm_prof_exit(a) mock_geopm_prof_exit(a)

    int return_zero(void)
    {
        return 0;
    }
    #define PMPI_Bsend(...) return_zero()
    #define PMPI_Bsend_init(...) return_zero()
    #define PMPI_Neighbor_allgather(...) return_zero()
    #define PMPI_Neighbor_allgatherv(...) return_zero()
    #define PMPI_Neighbor_alltoall(...) return_zero()
    #define PMPI_Neighbor_alltoallv(...) return_zero()
    #define PMPI_Neighbor_alltoallw(...) return_zero()
    #define PMPI_Cart_coords(...) return_zero()
    #define PMPI_Cart_create(...) return_zero()
    #define PMPI_Cart_get(...) return_zero()
    #define PMPI_Cart_map(...) return_zero()
    #define PMPI_Cart_rank(...) return_zero()
    #define PMPI_Cart_shift(...) return_zero()
    #define PMPI_Cart_sub(...) return_zero()
    #define PMPI_Cartdim_get(...) return_zero()
    #define PMPI_Comm_accept(...) return_zero()
    #define PMPI_Comm_connect(...) return_zero()
    #define PMPI_Comm_create_group(...) return_zero()
    #define PMPI_Comm_create(...) return_zero()
    #define PMPI_Comm_delete_attr(...) return_zero()
    #define PMPI_Comm_get_attr(...) return_zero()
    #define PMPI_Dist_graph_create(...) return_zero()
    #define PMPI_Dist_graph_create_adjacent(...) return_zero()
    #define PMPI_Dist_graph_neighbors(...) return_zero()
    #define PMPI_Dist_graph_neighbors_count(...) return_zero()
    #define PMPI_Comm_remote_group(...) return_zero()
    #define PMPI_Comm_remote_size(...) return_zero()
    #define PMPI_Comm_set_attr(...) return_zero()
    #define PMPI_Comm_set_info(...) return_zero()
    #define PMPI_Comm_spawn(...) return_zero()
    #define PMPI_Comm_spawn_multiple(...) return_zero()
    #define PMPI_File_open(...) return_zero()
    #define PMPI_Graph_get(...) return_zero()
    #define PMPI_Graph_map(...) return_zero()
    #define PMPI_Graph_neighbors_count(...) return_zero()
    #define PMPI_Graph_neighbors(...) return_zero()
    #define PMPI_Graphdims_get(...) return_zero()
    #define PMPI_Ibsend(...) return_zero()
    #define PMPI_Improbe(...) return_zero()
    #define PMPI_Intercomm_create(...) return_zero()
    #define PMPI_Intercomm_merge(...) return_zero()
    #define PMPI_Iprobe(...) return_zero()
    #define PMPI_Irecv(...) return_zero()
    #define PMPI_Irsend(...) return_zero()
    #define PMPI_Isend(...) return_zero()
    #define PMPI_Issend(...) return_zero()
    #define PMPI_Mprobe(...) return_zero()
    #define PMPI_Ineighbor_allgather(...) return_zero()
    #define PMPI_Ineighbor_allgatherv(...) return_zero()
    #define PMPI_Ineighbor_alltoall(...) return_zero()
    #define PMPI_Ineighbor_alltoallv(...) return_zero()
    #define PMPI_Ineighbor_alltoallw(...) return_zero()
    #define PMPI_Pack(...) return_zero()
    #define PMPI_Pack_size(...) return_zero()
    #define PMPI_Probe(...) return_zero()
    #define PMPI_Recv_init(...) return_zero()
    #define PMPI_Recv(...) return_zero()
    #define PMPI_Sendrecv(...) return_zero()
    #define PMPI_Sendrecv_replace(...) return_zero()
    #define PMPI_Ssend(...) return_zero()
    #define PMPI_Unpack(...) return_zero()
    #define PMPI_Win_allocate(...) return_zero()
    #define PMPI_Win_allocate_shared(...) return_zero()
    #define PMPI_Win_create(...) return_zero()
    #define PMPI_Win_create_dynamic(...) return_zero()
}

#include "gtest/gtest.h"
#include "geopm_pmpi.c"

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

class MPIInterfaceTest: public :: testing :: Test
{
    public:
        MPIInterfaceTest();
        ~MPIInterfaceTest();
    protected:
        void reset(void);
        void mpi_prof_check(void);
};

MPIInterfaceTest::MPIInterfaceTest()
{
    reset();
}


MPIInterfaceTest::~MPIInterfaceTest()
{
}

void MPIInterfaceTest::reset()
{
    // Reset globals in geopm_pmpi.c
    g_is_geopm_pmpi_ctl_enabled = 0;
    g_is_geopm_pmpi_prof_enabled = 0;
    G_GEOPM_COMM_WORLD_SWAP = MPI_COMM_WORLD;
    g_ppn1_comm = MPI_COMM_NULL;
    g_ctl = NULL;

    // Reset globals used in mocks
    g_test_curr_region_enter_id = 0;
    g_test_curr_region_exit_id = 0;
    g_test_curr_region_enter_count = 0;
    g_test_curr_region_exit_count = 0;

    geopm_pmpi_prof_enable(1);
}

void MPIInterfaceTest::mpi_prof_check()
{
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_enter_id);
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_exit_id);
    EXPECT_EQ(1, g_test_curr_region_enter_count);
    EXPECT_EQ(1, g_test_curr_region_exit_count);
    reset();
}

TEST_F(MPIInterfaceTest, geopm_api)
{
    // geopm_pmpi_prof_enable()
    geopm_pmpi_prof_enable(0);
    EXPECT_EQ(0, g_is_geopm_pmpi_prof_enabled);
    geopm_pmpi_prof_enable(0xDEADBEEF);
    EXPECT_EQ(0xDEADBEEF, g_is_geopm_pmpi_prof_enabled);
    reset();

    // geopm_swap_comm_world()
    // TODO GEOPM_PORTABLE_MPI_COMM_COMPARE_ENABLE testing
    MPI_Comm comm, result;
    result = geopm_swap_comm_world(comm);
    EXPECT_EQ(result, comm);
    EXPECT_NE(G_GEOPM_COMM_WORLD_SWAP, result);

    comm = MPI_COMM_WORLD;
    result = geopm_swap_comm_world(comm);
    EXPECT_EQ(G_GEOPM_COMM_WORLD_SWAP, result);
    reset();

    // geopm_mpi_region_enter()
    geopm_pmpi_prof_enable(1);
    geopm_mpi_region_enter();
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_enter_id);
    EXPECT_EQ(1, g_test_curr_region_enter_count);
    EXPECT_EQ(0, g_test_curr_region_exit_id);
    EXPECT_EQ(0, g_test_curr_region_exit_count);
    reset();

    // geopm_mpi_region_exit()
    geopm_pmpi_prof_enable(1);
    geopm_mpi_region_exit();
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_exit_id);
    EXPECT_EQ(1, g_test_curr_region_exit_count);
    EXPECT_EQ(0, g_test_curr_region_enter_id);
    EXPECT_EQ(0, g_test_curr_region_enter_count);
    reset();

    // geopm_pmpi_init()
    // TODO GEOPM_DEBUG<_ATTACH> testing
    // TODO setenv for GEOPM_PMPI_CTL_PROCESS
    // TODO setenv for GEOPM_PMPI_CTL_PTHREAD
    EXPECT_EQ(0, geopm_pmpi_init(NULL));
    reset();

    // geopm_pmpi_finalize()
    EXPECT_EQ(0, geopm_pmpi_finalize());
    reset();
}

TEST_F(MPIInterfaceTest, mpi_api)
{
    int junk = 0;
    int size, idummy;
    EXPECT_EQ(0, MPI_Comm_size(MPI_COMM_WORLD, &size));
    int zeros[size];
    memset(zeros, 0, size * sizeof(int));
    char buf[1000];
    void *dummy;

    MPI_Datatype dtypes[size];
    for (int i = 0; i < size; i++)
    {
        dtypes[i] = MPI_UNSIGNED;
    }

    EXPECT_EQ(0, MPI_Allgather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Allgatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros, zeros, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Allreduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Alltoall(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Alltoallv(&junk, zeros, zeros, MPI_UNSIGNED, NULL, zeros, zeros, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Alltoallw(&junk, zeros, zeros, dtypes, NULL, zeros, zeros, dtypes, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Barrier(MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Bcast(NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Bsend(NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Bsend_init(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();

    // MPI_Gather
    EXPECT_EQ(0, MPI_Gather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();

    // MPI_Gatherv
    EXPECT_EQ(0, MPI_Gatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros, zeros, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Neighbor_allgather(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Neighbor_allgatherv(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();
    
    EXPECT_EQ(0, MPI_Neighbor_alltoall(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Neighbor_alltoallv(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Neighbor_alltoallw(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    mpi_prof_check();
#endif

    EXPECT_EQ(0, MPI_Reduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, 0, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Reduce_scatter(NULL, NULL, zeros, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Reduce_scatter_block(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();
#endif

    EXPECT_EQ(0, MPI_Rsend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD));
    mpi_prof_check();

    MPI_Request req;
    EXPECT_EQ(0, MPI_Rsend_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Scan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Scatter(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Scatterv(NULL, zeros, zeros, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Waitall(0, NULL, NULL));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Waitany(0, NULL, &junk, NULL));
    mpi_prof_check();

    MPI_Status status;
    EXPECT_EQ(0, MPI_Wait(&req, &status));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Waitsome(0, NULL, &junk, NULL, NULL));
    mpi_prof_check();

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Iallgather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Iallgatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros, zeros, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Iallreduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ialltoall(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ialltoallv(&junk, zeros, zeros, MPI_UNSIGNED, NULL, zeros, zeros, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ialltoallw(&junk, zeros, zeros, dtypes, NULL, zeros, zeros, dtypes, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ibarrier(MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ibcast(NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
#endif

    EXPECT_EQ(0, MPI_Cart_coords(NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cart_create(NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cart_get(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cart_map(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cart_rank(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cart_shift(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cart_sub(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Cartdim_get(NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_accept(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(MPI_COMM_NULL, MPI_Comm_c2f(MPI_COMM_NULL));
    // Doing set_errhandler first so that MPI errors are not fatal
    EXPECT_EQ(0, MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN));
    EXPECT_EQ(0, MPI_Comm_call_errhandler(MPI_COMM_WORLD, 0));
    EXPECT_EQ(0, MPI_Comm_compare(MPI_COMM_WORLD, MPI_COMM_WORLD, &junk));
    EXPECT_EQ(0, MPI_Comm_connect(NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_create_group(NULL, NULL, NULL, NULL));
#endif

    EXPECT_EQ(0, MPI_Comm_create(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_delete_attr(NULL, NULL));

    MPI_Comm new_comm;
    EXPECT_EQ(0, MPI_Comm_dup(MPI_COMM_WORLD, &new_comm));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_idup(MPI_COMM_WORLD, &new_comm, &req));
    MPI_Info info;
    EXPECT_EQ(0, MPI_Comm_dup_with_info(MPI_COMM_WORLD, info, &new_comm));
#endif

    EXPECT_EQ(0, MPI_Comm_f2c(NULL));
    EXPECT_EQ(0, MPI_Comm_get_attr(NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Dist_graph_create(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Dist_graph_create_adjacent(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Dist_graph_neighbors(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Dist_graph_neighbors_count(NULL, NULL, NULL, NULL));
#endif

    MPI_Errhandler errhandlr;
    EXPECT_EQ(0, MPI_Comm_get_errhandler(MPI_COMM_WORLD, &errhandlr));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_get_info(MPI_COMM_WORLD, &info));
#endif

    EXPECT_EQ(0, MPI_Comm_get_name(MPI_COMM_WORLD, buf, &junk));
    EXPECT_EQ(0, MPI_Comm_get_parent(&new_comm));
    MPI_Group group;
    EXPECT_EQ(0, MPI_Comm_group(MPI_COMM_WORLD, &group));
    EXPECT_EQ(0, MPI_Comm_rank(MPI_COMM_WORLD, &junk));
    EXPECT_EQ(0, MPI_Comm_remote_group(NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_remote_size(NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_set_attr(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_set_errhandler(MPI_COMM_WORLD, errhandlr));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_set_info(NULL, NULL));
#endif

    EXPECT_EQ(0, MPI_Comm_set_name(MPI_COMM_WORLD, buf));
    EXPECT_EQ(0, MPI_Comm_size(MPI_COMM_WORLD, &junk));
    EXPECT_EQ(0, MPI_Comm_spawn(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_spawn_multiple(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &new_comm));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_split_type(MPI_COMM_WORLD,0, 0, info, &new_comm));
#endif

    EXPECT_EQ(0, MPI_Comm_test_inter(MPI_COMM_WORLD, &junk));

    EXPECT_EQ(0, MPI_Exscan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Iexscan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
#endif

    EXPECT_EQ(0, MPI_File_open(NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Igather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Igatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros, zeros, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
#endif

    EXPECT_EQ(0, MPI_Graph_create(MPI_COMM_WORLD, 0, zeros, zeros, 0, &new_comm));
    EXPECT_EQ(0, MPI_Graph_get(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Graph_map(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Graph_neighbors_count(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Graph_neighbors(NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Graphdims_get(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ibsend(NULL, NULL, NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Improbe(NULL, NULL, NULL, NULL, NULL, NULL));
#endif

    EXPECT_EQ(0, MPI_Intercomm_create(NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Intercomm_merge(NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Iprobe(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Irecv(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Irsend(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Isend(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Issend(NULL, NULL, NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Mprobe(NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ineighbor_allgather(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ineighbor_allgatherv(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ineighbor_alltoall(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ineighbor_alltoallv(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ineighbor_alltoallw(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
#endif

    EXPECT_EQ(0, MPI_Pack(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Pack_size(NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Probe(NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Recv_init(NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Recv(NULL, NULL, NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Ireduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, 0, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ireduce_scatter(NULL, NULL, zeros, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ireduce_scatter_block(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Iscan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Iscatter(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Iscatterv(NULL, zeros, zeros, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
#endif

    EXPECT_EQ(0, MPI_Send_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Send(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD));
    EXPECT_EQ(0, MPI_Sendrecv(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Sendrecv_replace(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Ssend_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    EXPECT_EQ(0, MPI_Ssend(NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Topo_test(MPI_COMM_WORLD, &junk));
    EXPECT_EQ(0, MPI_Unpack(NULL, NULL, NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Win_allocate(NULL, NULL, NULL, NULL, NULL, NULL));
    EXPECT_EQ(0, MPI_Win_allocate_shared(NULL, NULL, NULL, NULL, NULL, NULL));
#endif

    EXPECT_EQ(0, MPI_Win_create(NULL, NULL, NULL, NULL, NULL, NULL));

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Win_create_dynamic(NULL, NULL, NULL));
#endif
}

