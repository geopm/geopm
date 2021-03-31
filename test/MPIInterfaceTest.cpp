/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>

#include "gtest/gtest.h"

#include "config.h"

extern "C"
{
    typedef int MPI_Comm;
    typedef int MPI_Fint;
    typedef int MPI_Datatype;
    typedef int MPI_Request;
    typedef int MPI_Status;
    typedef int MPI_Op;
    typedef int MPI_Message;
    typedef int MPI_Aint;
    typedef int MPI_File;
    typedef int MPI_Win;
    typedef int MPI_Info;
    typedef int MPI_Group;
    typedef int MPI_Errhandler;
    #define MPI_COMM_WORLD 1
    #define MPI_COMM_NULL 0
    #define MPI_THREAD_MULTIPLE 1
    #define MPI_UNSIGNED 0
    #define MPI_MIN 0
    #define MPI_ERRORS_RETURN 1
    #define MPI_MAX_OBJECT_NAME 16
    #define MPI_INFO_NULL 0

    static int g_is_geopm_pmpi_ctl_enabled = 0;
    static MPI_Comm g_geopm_comm_world_swap = MPI_COMM_WORLD;
    static MPI_Comm g_ppn1_comm = MPI_COMM_NULL;
    static struct geopm_ctl_c *g_ctl = NULL;

    MPI_Comm geopm_swap_comm_world(MPI_Comm comm)
    {
        return comm != MPI_COMM_WORLD ?
               comm : g_geopm_comm_world_swap;
    }

    // will be set to MPI region ID in test setup
    uint64_t G_EXPECTED_REGION_ID = 1234;

    int geopm_is_pmpi_prof_enabled(void)
    {
        return 1;
    }

    static uint64_t g_test_curr_region_enter_id = 0;
    static int g_test_curr_region_enter_count = 0;
    int mock_geopm_prof_enter(uint64_t region_id)
    {
        g_test_curr_region_enter_id = region_id;
        g_test_curr_region_enter_count++;
        return 0;
    }
    #define geopm_prof_enter(a) mock_geopm_prof_enter(a)

    static uint64_t g_test_curr_region_exit_id = 0;
    static int g_test_curr_region_exit_count = 0;
    int mock_geopm_prof_exit(uint64_t region_id)
    {
        g_test_curr_region_exit_id = region_id;
        g_test_curr_region_exit_count++;
        return 0;
    }
    #define geopm_prof_exit(a) mock_geopm_prof_exit(a)


    MPI_Comm g_passed_comm_arg = MPI_COMM_WORLD;

    int return_zero(void)
    {
        return 0;
    }

    #define MPI_Abort(...) return_zero()
    #define PMPI_Comm_c2f(...) return_zero()
    #define PMPI_Comm_f2c(...) return_zero()
    #define PMPI_Comm_free(...) return_zero()
    #define PMPI_Comm_get_parent(...) return_zero()
    #define PMPI_Finalize(...) return_zero()
    #define PMPI_Init(...) return_zero()
    #define PMPI_Wait(...) return_zero()
    #define PMPI_Waitsome(...) return_zero()
    #define PMPI_Waitany(...) return_zero()
    #define PMPI_Waitall(...) return_zero()

    #define geopm_ctl_create(...) return_zero()
    #define geopm_ctl_run(...) return_zero()
    #define geopm_ctl_pthread(...) return_zero()
    #define geopm_ctl_destroy(...) return_zero()

    #include "pmpi_mock.c"

    int MPI_Finalize();
    int MPI_Comm_rank(MPI_Comm comm, int *rank);

    int PMPI_Query_thread(int *mpi_thread_level) {
        *mpi_thread_level = 1;
        return 0;
    }
    int PMPI_Init_thread(int *argc, char **argv[], int required, int *provided) {
        *provided = 1;
        return 0;
    }

    int geopm_comm_split(MPI_Comm comm, const char *tag, MPI_Comm *split_comm, int *is_ctl_comm) {
        *split_comm = 2;
        *is_ctl_comm = 1;
        return 0;
    }

    int geopm_comm_split_ppn1(MPI_Comm comm, const char *tag, MPI_Comm *ppn1_comm) {
        return 0;
    }

#include "geopm_internal.h"

    void geopm_mpi_region_enter(uint64_t func_rid)
    {
        if (func_rid) {
            geopm_prof_enter(func_rid);
        }
        geopm_prof_enter(GEOPM_REGION_ID_MPI);
    }

    void geopm_mpi_region_exit(uint64_t func_rid)
    {
        geopm_prof_exit(GEOPM_REGION_ID_MPI);
        if (func_rid) {
            geopm_prof_exit(func_rid);
        }
    }
    int geopm_prof_region(const char *region_name, uint64_t hint, uint64_t *region_id) {
        *region_id = G_EXPECTED_REGION_ID;
        return 0;
    }

    uint64_t geopm_mpi_func_rid(const char *func_name)
    {
        uint64_t result = 0;
        (void)geopm_prof_region(func_name, 0x0, &result);
        return result;
    }
} // end extern C

#define GEOPM_TEST
#include "geopm_pmpi.c"

class MPIInterfaceTest: public :: testing :: Test
{
    public:
        MPIInterfaceTest();
        ~MPIInterfaceTest();
    protected:
        void reset(void);
        void mpi_prof_check(void);
        void comm_swap_check(int line);
};

MPIInterfaceTest::MPIInterfaceTest()
{
    G_EXPECTED_REGION_ID = GEOPM_REGION_ID_MPI;
    reset();
}

MPIInterfaceTest::~MPIInterfaceTest()
{
}

void MPIInterfaceTest::reset()
{
    // Reset globals in geopm_pmpi.c
    g_is_geopm_pmpi_ctl_enabled = 0;
    g_ppn1_comm = MPI_COMM_NULL;
    g_ctl = NULL;

    // Reset globals used in mocks
    g_test_curr_region_enter_id = 0;
    g_test_curr_region_exit_id = 0;
    g_test_curr_region_enter_count = 0;
    g_test_curr_region_exit_count = 0;

    // mock initialization
    g_geopm_comm_world_swap = MPI_COMM_WORLD + 1;
}

void MPIInterfaceTest::mpi_prof_check()
{
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_enter_id);
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_exit_id);
    EXPECT_EQ(2, g_test_curr_region_enter_count);
    EXPECT_EQ(2, g_test_curr_region_exit_count);
    reset();
}

void MPIInterfaceTest::comm_swap_check(int line)
{
    if (g_passed_comm_arg == MPI_COMM_WORLD) {
        FAIL() << "Passed comm was equal to MPI_COMM_WORLD near line " << line << std::endl;
    }
    g_passed_comm_arg = MPI_COMM_WORLD;
}

TEST_F(MPIInterfaceTest, geopm_api)
{
    // TODO GEOPM_PORTABLE_MPI_COMM_COMPARE_ENABLE testing
    MPI_Comm comm = MPI_COMM_NULL, result;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    result = geopm_swap_comm_world(comm);
    EXPECT_EQ(result, comm);
    EXPECT_NE(g_geopm_comm_world_swap, result);

    comm = MPI_COMM_WORLD;
    result = geopm_swap_comm_world(comm);
    EXPECT_EQ(g_geopm_comm_world_swap, result);
    reset();

    geopm_mpi_region_enter(0);
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_enter_id);
    EXPECT_EQ(1, g_test_curr_region_enter_count);
    EXPECT_EQ((uint64_t)0, g_test_curr_region_exit_id);
    EXPECT_EQ(0, g_test_curr_region_exit_count);
    reset();

    geopm_mpi_region_exit(0);
    EXPECT_EQ(GEOPM_REGION_ID_MPI, g_test_curr_region_exit_id);
    EXPECT_EQ(1, g_test_curr_region_exit_count);
    EXPECT_EQ((uint64_t)0, g_test_curr_region_enter_id);
    EXPECT_EQ(0, g_test_curr_region_enter_count);
    reset();
}

TEST_F(MPIInterfaceTest, mpi_api)
{
    int junk = 0;
    int size = 100;
    EXPECT_EQ(0, MPI_Comm_size(MPI_COMM_WORLD, &size));
    std::vector<int> zeros(size);
    std::fill(zeros.begin(), zeros.end(), 0);
    char sbuf[MPI_MAX_OBJECT_NAME];
    char *dbuf;
    char **ddbuf;
    MPI_Request req = 0;
    MPI_Info info = MPI_INFO_NULL;
    MPI_Group group = 0;
    MPI_Comm comm = 0;
    std::vector<MPI_Datatype> dtypes(size);
    std::fill(dtypes.begin(), dtypes.end(), MPI_UNSIGNED);
    MPI_Aint aint = 0;
    std::vector<MPI_Request> reqs(size);
    MPI_Status status = 0;
    std::vector<MPI_Status> statuses(size);
    MPI_File fh = 0;
    MPI_Win win = 0;
#ifdef GEOPM_ENABLE_MPI3
    MPI_Aint aints[size];
    MPI_Message message = 0;
#endif

    MPI_Comm_group(MPI_COMM_WORLD, &group);
    reset();

    EXPECT_EQ(0, MPI_Allgather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Allgatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Allreduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Alltoall(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Alltoallv(&junk, zeros.data(), zeros.data(), MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Alltoallw(&junk, zeros.data(), zeros.data(), dtypes.data(), NULL, zeros.data(), zeros.data(), dtypes.data(), MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Barrier(MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Bcast(NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Bsend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_NULL));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Bsend_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_NULL, &req));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Gather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Gatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Neighbor_allgather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Neighbor_allgatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Neighbor_alltoall(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Neighbor_alltoallv(NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Neighbor_alltoallw(NULL, zeros.data(), aints, dtypes.data(), NULL, zeros.data(), aints, dtypes.data(), MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);
#endif

    EXPECT_EQ(0, MPI_Reduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Reduce_scatter(NULL, NULL, zeros.data(), MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Reduce_scatter_block(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);
#endif

    EXPECT_EQ(0, MPI_Rsend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Rsend_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Scan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Scatter(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Scatterv(NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD));
    mpi_prof_check();
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Waitall(0, reqs.data(), &status));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Waitany(0, reqs.data(), &junk, &status));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Wait(&req, &status));
    mpi_prof_check();

    EXPECT_EQ(0, MPI_Waitsome(0, reqs.data(), &junk, zeros.data(), statuses.data()));
    mpi_prof_check();

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Iallgather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Iallgatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Iallreduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ialltoall(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ialltoallv(&junk, zeros.data(), zeros.data(), MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ialltoallw(&junk, zeros.data(), zeros.data(), dtypes.data(), NULL, zeros.data(), zeros.data(), dtypes.data(), MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ibarrier(MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ibcast(NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Cart_coords(MPI_COMM_WORLD, 0, 0, zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cart_create(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), 0, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cart_get(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cart_map(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cart_rank(MPI_COMM_WORLD, zeros.data(), &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cart_shift(MPI_COMM_WORLD, 0, 0, &junk, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cart_sub(MPI_COMM_WORLD, zeros.data(), &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Cartdim_get(MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_accept(sbuf, info, 0, MPI_COMM_WORLD, &comm));
    comm_swap_check(__LINE__);

    // Doing set_errhandler first so that MPI errors are not fatal
    EXPECT_EQ(0, MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_call_errhandler(MPI_COMM_WORLD, 0));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_compare(MPI_COMM_WORLD, MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_connect(sbuf, info, 0, MPI_COMM_WORLD, &comm));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_create_group(MPI_COMM_WORLD, group, 0, &comm));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Comm_create(MPI_COMM_WORLD, group, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_delete_attr(MPI_COMM_WORLD, 0));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_dup(MPI_COMM_WORLD, &comm));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_idup(MPI_COMM_WORLD, &comm, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_dup_with_info(MPI_COMM_WORLD, info, &comm));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Comm_get_attr(MPI_COMM_WORLD, 0, NULL, &junk));
    comm_swap_check(__LINE__);

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Dist_graph_create(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), zeros.data(), zeros.data(), info, 0, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Dist_graph_create_adjacent(comm, 0, zeros.data(), zeros.data(), 0, zeros.data(), zeros.data(), info, 0, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Dist_graph_neighbors(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), 0, zeros.data(), zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Dist_graph_neighbors_count(MPI_COMM_WORLD, &junk, &junk, &junk));
    comm_swap_check(__LINE__);

#endif

    MPI_Errhandler errhandlr;
    EXPECT_EQ(0, MPI_Comm_get_errhandler(MPI_COMM_WORLD, &errhandlr));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_get_info(MPI_COMM_WORLD, &info));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Comm_get_name(MPI_COMM_WORLD, sbuf, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_group(MPI_COMM_WORLD, &group));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_rank(MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_remote_group(MPI_COMM_WORLD, &group));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_remote_size(MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_set_attr(MPI_COMM_WORLD, 0, NULL));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_set_errhandler(MPI_COMM_WORLD, errhandlr));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_set_info(MPI_COMM_WORLD, info));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Comm_set_name(MPI_COMM_WORLD, sbuf));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_size(MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_spawn(sbuf, &dbuf, 0, info, 0, MPI_COMM_WORLD, &comm, zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_spawn_multiple(0, &dbuf, &ddbuf, zeros.data(), &info, 0, MPI_COMM_WORLD, &comm, zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Comm_split(MPI_COMM_WORLD, 0, 0, &comm));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Comm_split_type(MPI_COMM_WORLD, 0, 0, info, &comm));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Comm_test_inter(MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Exscan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD));
    comm_swap_check(__LINE__);

#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Iexscan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_File_open(MPI_COMM_WORLD, sbuf, 0, info, &fh));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Igather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Igatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Graph_create(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), 0, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Graph_get(MPI_COMM_WORLD, 0, 0, zeros.data(), zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Graph_map(MPI_COMM_WORLD, 0, zeros.data(), zeros.data(), &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Graph_neighbors_count(MPI_COMM_WORLD, 0, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Graph_neighbors(MPI_COMM_WORLD, 0, 0, zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Graphdims_get(MPI_COMM_WORLD, zeros.data(), zeros.data()));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ibsend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Improbe(0, 0, MPI_COMM_WORLD, &junk, &message, &status));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Intercomm_create(comm, 0, comm, 0, 0, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Intercomm_merge(MPI_COMM_WORLD, 0, &comm));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Iprobe(0, 0, MPI_COMM_WORLD, &junk, &status));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Irecv(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Irsend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Isend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Issend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Mprobe(0, 0, MPI_COMM_WORLD, &message, &status));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ineighbor_allgather(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ineighbor_allgatherv(NULL, 0, MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ineighbor_alltoall(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ineighbor_alltoallv(NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ineighbor_alltoallw(NULL, zeros.data(), aints, dtypes.data(), NULL, zeros.data(), aints, dtypes.data(), MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Pack(NULL, 0, MPI_UNSIGNED, NULL, 0, &junk, MPI_COMM_WORLD));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Pack_size(0, MPI_UNSIGNED, MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Probe(0, 0, MPI_COMM_WORLD, &status));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Recv_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Recv(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &status));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Ireduce(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ireduce_scatter(NULL, NULL, zeros.data(), MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ireduce_scatter_block(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Iscan(NULL, NULL, 0, MPI_UNSIGNED, MPI_MIN, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Iscatter(NULL, 0, MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Iscatterv(NULL, zeros.data(), zeros.data(), MPI_UNSIGNED, NULL, 0, MPI_UNSIGNED, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

#endif

    EXPECT_EQ(0, MPI_Send_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Send(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Sendrecv(NULL, 0, MPI_UNSIGNED, 0, 0, NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &status));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Sendrecv_replace(NULL, 0, MPI_UNSIGNED, 0, 0, 0, 0, MPI_COMM_WORLD, &status));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ssend_init(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD, &req));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Ssend(NULL, 0, MPI_UNSIGNED, 0, 0, MPI_COMM_WORLD));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Topo_test(MPI_COMM_WORLD, &junk));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Unpack(NULL, 0, &junk, NULL, 0, MPI_UNSIGNED, MPI_COMM_WORLD));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Win_allocate(aint, 0, info, MPI_COMM_WORLD, NULL, &win));
    comm_swap_check(__LINE__);

    EXPECT_EQ(0, MPI_Win_allocate_shared(aint, 0, info, MPI_COMM_WORLD, NULL, &win));
    comm_swap_check(__LINE__);
#endif

    EXPECT_EQ(0, MPI_Win_create(NULL, aint, 0, info, MPI_COMM_WORLD, &win));
    comm_swap_check(__LINE__);


#ifdef GEOPM_ENABLE_MPI3
    EXPECT_EQ(0, MPI_Win_create_dynamic(info, MPI_COMM_WORLD, &win));
    comm_swap_check(__LINE__);
#endif
}
