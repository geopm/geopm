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

#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>

#include "config.h"

/* GEOPM wrap functions */
extern MPI_Fint geopm_swap_comm_world_f(MPI_Fint comm);
extern void geopm_mpi_region_enter(uint64_t func_rid);
extern void geopm_mpi_region_exit(uint64_t func_rid);
extern uint64_t geopm_mpi_func_rid(const char *func_name);

#define GEOPM_MPI_ENTER_MACRO(FUNC) \
    static unsigned is_once = 1; \
    static uint64_t func_rid = 0; \
    if (is_once) { \
        func_rid = geopm_mpi_func_rid(FUNC); \
        is_once = 0; \
    } \
    geopm_mpi_region_enter(func_rid);

#define GEOPM_MPI_EXIT_MACRO geopm_mpi_region_exit(func_rid);

/* MPI FORTRAN extern definitions */
#ifdef GEOPM_ENABLE_MPI3
extern void pmpi_comm_create_group_(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* tag, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_comm_get_info_(MPI_Fint* comm, MPI_Fint* info_used, MPI_Fint* ierr);
extern void pmpi_comm_idup_(MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_comm_set_info_(MPI_Fint* comm, MPI_Fint* info, MPI_Fint* ierr);
extern void pmpi_comm_split_type_(MPI_Fint* comm, MPI_Fint* split_type, MPI_Fint* key, MPI_Fint* info, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_dist_graph_create_adjacent_(MPI_Fint* comm_old, MPI_Fint* indegree, MPI_Fint* sources, MPI_Fint* sourceweights, MPI_Fint* outdegree, MPI_Fint* destinations, MPI_Fint* destweights, MPI_Fint* info, MPI_Fint* reorder, MPI_Fint* comm_dist_graph, MPI_Fint* ierr);
extern void pmpi_dist_graph_create_(MPI_Fint* comm_old, MPI_Fint* n, MPI_Fint* nodes, MPI_Fint* degrees, MPI_Fint* targets, MPI_Fint* weights, MPI_Fint* info, MPI_Fint* reorder, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_dist_graph_neighbors_count_(MPI_Fint* comm, MPI_Fint* inneighbors, MPI_Fint* outneighbors, MPI_Fint* weighted, MPI_Fint* ierr);
extern void pmpi_dist_graph_neighbors(MPI_Fint* comm, MPI_Fint* maxindegree, MPI_Fint* sources, MPI_Fint* sourceweights, MPI_Fint* maxoutdegree, MPI_Fint* destinations, MPI_Fint* destweights, MPI_Fint* ierr);
extern void pmpi_iallgather_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_iallgatherv_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* displs, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_iallreduce_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ialltoall_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ialltoallv_(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Fint* sdispls, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* rdispls, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ialltoallw_(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Fint* sdispls, MPI_Fint* sendtypes, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* rdispls, MPI_Fint* recvtypes, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ibarrier_(MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ibcast_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_iexscan_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_igather_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_igatherv_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* displs, MPI_Fint* recvtype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_improbe_(MPI_Fint* source, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* flag, MPI_Fint* message, MPI_Fint* status, MPI_Fint* ierr);
extern void pmpi_iprobe_(MPI_Fint* source, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* flag, MPI_Fint* status, MPI_Fint* ierr);
extern void pmpi_ireduce_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ireduce_scatter_block_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ireduce_scatter_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_iscan_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_iscatter_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_iscatterv_(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Fint* displs, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_neighbor_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_neighbor_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_neighbor_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_neighbor_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_neighbor_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_reduce_scatter_block_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_win_allocate_(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr);
extern void pmpi_win_allocate_shared_(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr);
extern void pmpi_win_create_dynamic_(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr);
#endif
extern void pmpi_allgather_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_allgatherv_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* displs, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_allreduce_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_alltoall_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_alltoallv_(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Fint* sdispls, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* rdispls, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_alltoallw(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Fint* sdispls, MPI_Fint* sendtypes, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* rdispls, MPI_Fint* recvtypes, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_barrier_(MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_bcast_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_bsend_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* dest, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_bsend_init_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* dest, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_cart_coords_(MPI_Fint* comm, MPI_Fint* rank, MPI_Fint* maxdims, MPI_Fint* coords, MPI_Fint* ierr);
extern void pmpi_cart_create_(MPI_Fint* old_comm, MPI_Fint* ndims, MPI_Fint* dims, MPI_Fint* periods, MPI_Fint* reorder, MPI_Fint* comm_cart, MPI_Fint* ierr);
extern void pmpi_cartdim_get_(MPI_Fint* comm, MPI_Fint* ndims, MPI_Fint* ierr);
extern void pmpi_cart_get_(MPI_Fint* comm, MPI_Fint* maxdims, MPI_Fint* dims, MPI_Fint* periods, MPI_Fint* coords, MPI_Fint* ierr);
extern void pmpi_cart_map(MPI_Fint* comm, MPI_Fint* ndims, MPI_Fint* dims, MPI_Fint* periods, MPI_Fint* newrank, MPI_Fint* ierr);
extern void pmpi_cart_rank_(MPI_Fint* comm, MPI_Fint* coords, MPI_Fint* rank, MPI_Fint* ierr);
extern void pmpi_cart_shift_(MPI_Fint* comm, MPI_Fint* direction, MPI_Fint* disp, MPI_Fint* rank_source, MPI_Fint* rank_dest, MPI_Fint* ierr);
extern void pmpi_cart_sub_(MPI_Fint* comm, MPI_Fint* remain_dims, MPI_Fint* new_comm, MPI_Fint* ierr);
extern void pmpi_comm_accept_(char* port_name, MPI_Fint* info, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint* ierr, MPI_Fint port_name_len);
extern void pmpi_comm_call_errhandler_(MPI_Fint* comm, MPI_Fint* errorcode, MPI_Fint* ierr);
extern void pmpi_comm_compare_(MPI_Fint* comm1, MPI_Fint* comm2, MPI_Fint* result, MPI_Fint* ierr);
extern void pmpi_comm_connect_(char* port_name, MPI_Fint* info, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint* ierr, MPI_Fint port_name_len);
extern void pmpi_comm_create_(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_comm_delete_attr_(MPI_Fint* comm, MPI_Fint* comm_keyval, MPI_Fint* ierr);
extern void pmpi_comm_dup(MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_comm_dup_with_info_(MPI_Fint* comm, MPI_Fint* info, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_comm_get_attr_(MPI_Fint* comm, MPI_Fint* comm_keyval, MPI_Fint* attribute_val, MPI_Fint* flag, MPI_Fint* ierr);
extern void pmpi_comm_get_errhandler_(MPI_Fint* comm, MPI_Fint* erhandler, MPI_Fint* ierr);
extern void pmpi_comm_get_name_(MPI_Fint* comm, char* comm_name, MPI_Fint* resultlen, MPI_Fint* ierr, MPI_Fint name_len);
extern void pmpi_comm_group_(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* ierr);
extern void pmpi_comm_rank_(MPI_Fint* comm, MPI_Fint* rank, MPI_Fint* ierr);
extern void pmpi_comm_remote_group_(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* ierr);
extern void pmpi_comm_remote_size_(MPI_Fint* comm, MPI_Fint* size, MPI_Fint* ierr);
extern void pmpi_comm_set_attr_(MPI_Fint* comm, MPI_Fint* comm_keyval, MPI_Fint* attribute_val, MPI_Fint* ierr);
extern void pmpi_comm_set_errhandler_(MPI_Fint* comm, MPI_Fint* errhandler, MPI_Fint* ierr);
extern void pmpi_comm_set_name(MPI_Fint* comm, char* comm_name, MPI_Fint* ierr, MPI_Fint name_len);
extern void pmpi_comm_size_(MPI_Fint* comm, MPI_Fint* size, MPI_Fint* ierr);
extern void pmpi_comm_spawn_(char* command, char* argv, MPI_Fint* maxprocs, MPI_Fint* info, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* intercomm, MPI_Fint* array_of_errcodes, MPI_Fint* ierr, MPI_Fint cmd_len, MPI_Fint string_len);
extern void pmpi_comm_spawn_multiple_(MPI_Fint* count, char* array_of_commands, char* array_of_argv, MPI_Fint* array_of_maxprocs, MPI_Fint* array_of_info, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* intercomm, MPI_Fint* array_of_errcodes, MPI_Fint* ierr, MPI_Fint cmd_string_len, MPI_Fint argv_string_len);
extern void pmpi_comm_split(MPI_Fint* comm, MPI_Fint* color, MPI_Fint* key, MPI_Fint* newcomm, MPI_Fint* ierr);
extern void pmpi_comm_test_inter_(MPI_Fint* comm, MPI_Fint* flag, MPI_Fint* ierr);
extern void pmpi_exscan_(MPI_Fint* sendbuf, MPI_Fint* recvbuf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_file_open_(MPI_Fint* comm, char* filename, MPI_Fint* amode, MPI_Fint* info, MPI_Fint* fh, MPI_Fint* ierr, MPI_Fint name_len);
extern void pmpi_gather_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_gatherv_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* displs, MPI_Fint* recvtype, MPI_Fint* root, MPI_Fint* comm, MPI_Fint* ierr);
extern void pmpi_graph_create_(MPI_Fint* comm_old, MPI_Fint* nnodes, MPI_Fint* index, MPI_Fint* edges, MPI_Fint* reorder, MPI_Fint* comm_graph, MPI_Fint* ierr);
extern void pmpi_graphdims_get(MPI_Fint* comm, MPI_Fint* nnodes, MPI_Fint* nedges, MPI_Fint* ierr);
extern void pmpi_graph_get(MPI_Fint* comm, MPI_Fint* maxindex, MPI_Fint* maxedges, MPI_Fint* index, MPI_Fint* edges, MPI_Fint* ierr);
extern void pmpi_graph_map_(MPI_Fint* comm, MPI_Fint* nnodes, MPI_Fint* index, MPI_Fint* edges, MPI_Fint* newrank, MPI_Fint* ierr);
extern void pmpi_graph_neighbors_count_(MPI_Fint* comm, MPI_Fint* rank, MPI_Fint* nneighbors, MPI_Fint* ierr);
extern void pmpi_graph_neighbors_(MPI_Fint* comm, MPI_Fint* rank, MPI_Fint* maxneighbors, MPI_Fint* neighbors, MPI_Fint* ierr);
extern void pmpi_ibsend_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* dest, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ineighbor_allgather_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ineighbor_allgatherv_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* displs, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ineighbor_alltoall_(MPI_Fint* sendbuf, MPI_Fint* sendcount, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ineighbor_alltoallv_(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Fint* sdispls, MPI_Fint* sendtype, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Fint* rdispls, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_ineighbor_alltoallw_(MPI_Fint* sendbuf, MPI_Fint* sendcounts, MPI_Aint* sdispls, MPI_Fint* sendtypes, MPI_Fint* recvbuf, MPI_Fint* recvcounts, MPI_Aint* rdispls, MPI_Fint* recvtypes, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_intercomm_create_(MPI_Fint* local_comm, MPI_Fint* local_leader, MPI_Fint* bridge_comm, MPI_Fint* remote_leader, MPI_Fint* tag, MPI_Fint* newintercomm, MPI_Fint* ierr);
extern void pmpi_intercomm_merge_(MPI_Fint* intercomm, MPI_Fint* high, MPI_Fint* newintercomm, MPI_Fint* ierr);
extern void pmpi_irecv_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* source, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_irsend_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* dest, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_isend_(MPI_Fint* buf, MPI_Fint* count, MPI_Fint* datatype, MPI_Fint* dest, MPI_Fint* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint* ierr);
extern void pmpi_unpack_(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_comm_get_parent_(MPI_Fint *parent, MPI_Fint *ierr);
extern void pmpi_issend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr);
extern void pmpi_mprobe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_pack_(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_pack_size_(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr);
extern void pmpi_probe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_recv_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr);
extern void pmpi_recv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_reduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_reduce_scatter_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_rsend_(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_rsend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr);
extern void pmpi_scan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_scatter_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_scatterv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_send_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_send_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr);
extern void pmpi_sendrecv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_sendrecv_replace_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_ssend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_ssend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr);
extern void pmpi_topo_test_(MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_unpack_(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr);
extern void pmpi_waitall_(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierr);
extern void pmpi_waitany_(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr);
extern void pmpi_waitsome_(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr);
extern void pmpi_win_create_(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr);

/* MPI_ALLGATHER Fortran wrappers */
void mpi_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_allgather_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_ALLGATHERV Fortran wrappers */
void mpi_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_allgatherv_(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_ALLREDUCE Fortran wrappers */
void mpi_allreduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_allreduce_(sendbuf, recvbuf, count, datatype, op, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_ALLTOALL Fortran wrappers */
void mpi_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_alltoall_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_ALLTOALLV Fortran wrappers */
void mpi_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_alltoallv_(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_ALLTOALLW Fortran wrappers */
void mpi_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_BARRIER Fortran wrappers */
void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_barrier_(&comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_BCAST Fortran wrappers */
void mpi_bcast_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_bcast_(buf, count, datatype, root, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_BSEND Fortran wrappers */
void mpi_bsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_bsend_(buf, count, datatype, dest, tag, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_BSEND_INIT Fortran wrappers */
void mpi_bsend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_bsend_init_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_CART_COORDS Fortran wrappers */
void mpi_cart_coords_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxdims, MPI_Fint *coords, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cart_coords_(&comm_swap, rank, maxdims, coords, ierr);
}

/* MPI_CART_CREATE Fortran wrappers */
void mpi_cart_create_(MPI_Fint *old_comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierr)
{
    MPI_Fint old_comm_swap = geopm_swap_comm_world_f(*old_comm);
    pmpi_cart_create_(&old_comm_swap, ndims, dims, periods, reorder, comm_cart, ierr);
}

/* MPI_CARTDIM_GET Fortran wrappers */
void mpi_cartdim_get_(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cartdim_get_(&comm_swap, ndims, ierr);
}

/* MPI_CART_GET Fortran wrappers */
void mpi_cart_get_(MPI_Fint *comm, MPI_Fint *maxdims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *coords, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cart_get_(&comm_swap, maxdims, dims, periods, coords, ierr);
}

/* MPI_CART_MAP Fortran wrappers */
void mpi_cart_map_(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *newrank, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cart_map(&comm_swap, ndims, dims, periods, newrank, ierr);
}

/* MPI_CART_RANK Fortran wrappers */
void mpi_cart_rank_(MPI_Fint *comm, MPI_Fint *coords, MPI_Fint *rank, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cart_rank_(&comm_swap, coords, rank, ierr);
}

/* MPI_CART_SHIFT Fortran wrappers */
void mpi_cart_shift_(MPI_Fint *comm, MPI_Fint *direction, MPI_Fint *disp, MPI_Fint *rank_source, MPI_Fint *rank_dest, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cart_shift_(&comm_swap, direction, disp, rank_source, rank_dest, ierr);
}

/* MPI_CART_SUB Fortran wrappers */
void mpi_cart_sub_(MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *new_comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_cart_sub_(&comm_swap, remain_dims, new_comm, ierr);
}

/* MPI_COMM_ACCEPT Fortran wrappers */
void mpi_comm_accept_(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_accept_(port_name, info, root, &comm_swap, newcomm, ierr, port_name_len);
}

/* MPI_COMM_CALL_ERRHANDLER Fortran wrappers */
void mpi_comm_call_errhandler_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_call_errhandler_(&comm_swap, errorcode, ierr);
}

/* MPI_COMM_COMPARE Fortran wrappers */
void mpi_comm_compare_(MPI_Fint *comm1, MPI_Fint *comm2, MPI_Fint *result, MPI_Fint *ierr)
{
    MPI_Fint comm1_swap = geopm_swap_comm_world_f(*comm1);
    MPI_Fint comm2_swap = geopm_swap_comm_world_f(*comm2);
    pmpi_comm_compare_(&comm1_swap, &comm2_swap, result, ierr);
}

/* MPI_COMM_CONNECT Fortran wrappers */
void mpi_comm_connect_(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_connect_(port_name, info, root, &comm_swap, newcomm, ierr, port_name_len);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_COMM_CREATE_GROUP Fortran wrappers */
void mpi_comm_create_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *tag, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_create_group_(&comm_swap, group, tag, newcomm, ierr);
}
#endif

/* MPI_COMM_CREATE Fortran wrappers */
void mpi_comm_create_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_create_(&comm_swap, group, newcomm, ierr);
}

/* MPI_COMM_DELETE_ATTR Fortran wrappers */
void mpi_comm_delete_attr_(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_delete_attr_(&comm_swap, comm_keyval, ierr);
}

/* MPI_COMM_DUP Fortran wrappers */
void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_dup(&comm_swap, newcomm, ierr);
}

/* MPI_COMM_DUP_WITH_INFO Fortran wrappers */
void mpi_comm_dup_with_info_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_dup_with_info_(&comm_swap, info, newcomm, ierr);
}

/* MPI_COMM_GET_ATTR Fortran wrappers */
void mpi_comm_get_attr_(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *flag, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_get_attr_(&comm_swap, comm_keyval, attribute_val, flag, ierr);
}

/* MPI_COMM_GET_ERRHANDLER Fortran wrappers */
void mpi_comm_get_errhandler_(MPI_Fint *comm, MPI_Fint *erhandler, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_get_errhandler_(&comm_swap, erhandler, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_COMM_GET_INFO Fortran wrappers */
void mpi_comm_get_info_(MPI_Fint *comm, MPI_Fint *info_used, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_get_info_(&comm_swap, info_used, ierr);
}
#endif

/* MPI_COMM_GET_NAME Fortran wrappers */
void mpi_comm_get_name_(MPI_Fint *comm, char *comm_name, MPI_Fint *resultlen, MPI_Fint *ierr, int name_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_get_name_(&comm_swap, comm_name, resultlen, ierr, name_len);
}

/* MPI_COMM_GET_PARENT Fortran wrappers */
void mpi_comm_get_parent_(MPI_Fint *parent, MPI_Fint *ierr)
{
    pmpi_comm_get_parent_(parent, ierr);
}

/* MPI_COMM_GROUP Fortran wrappers */
void mpi_comm_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_group_(&comm_swap, group, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_COMM_IDUP Fortran wrappers */
void mpi_comm_idup_(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_idup_(&comm_swap, newcomm, request, ierr);
}
#endif

/* MPI_COMM_RANK Fortran wrappers */
void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_rank_(&comm_swap, rank, ierr);
}

/* MPI_COMM_REMOTE_GROUP Fortran wrappers */
void mpi_comm_remote_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_remote_group_(&comm_swap, group, ierr);
}

/* MPI_COMM_REMOTE_SIZE Fortran wrappers */
void mpi_comm_remote_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_remote_size_(&comm_swap, size, ierr);
}

/* MPI_COMM_SET_ATTR Fortran wrappers */
void mpi_comm_set_attr_(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_set_attr_(&comm_swap, comm_keyval, attribute_val, ierr);
}

/* MPI_COMM_SET_ERRHANDLER Fortran wrappers */
void mpi_comm_set_errhandler_(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_set_errhandler_(&comm_swap, errhandler, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_COMM_SET_INFO Fortran wrappers */
void mpi_comm_set_info_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_set_info_(&comm_swap, info, ierr);
}
#endif

/* MPI_COMM_SET_NAME Fortran wrappers */
void mpi_comm_set_name_(MPI_Fint *comm,  char *comm_name, MPI_Fint *ierr, int name_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_set_name(&comm_swap, comm_name, ierr, name_len);
}

/* MPI_COMM_SIZE Fortran wrappers */
void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_size_(&comm_swap, size, ierr);
}

/* MPI_COMM_SPAWN Fortran wrappers */
void mpi_comm_spawn_(char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_len, int string_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_spawn_(command, argv, maxprocs, info, root, &comm_swap, intercomm, array_of_errcodes, ierr, cmd_len, string_len);
}

/* MPI_COMM_SPAWN_MULTIPLE Fortran wrappers */
void mpi_comm_spawn_multiple_(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI_Fint *array_of_maxprocs, MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_string_len, int argv_string_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_spawn_multiple_(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, &comm_swap, intercomm, array_of_errcodes, ierr, cmd_string_len, argv_string_len);
}

/* MPI_COMM_SPLIT Fortran wrappers */
void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_split(&comm_swap, color, key, newcomm, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_COMM_SPLIT_TYPE Fortran wrappers */
void mpi_comm_split_type_(MPI_Fint *comm, MPI_Fint *split_type, MPI_Fint *key, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_split_type_(&comm_swap, split_type, key, info, newcomm, ierr);
}
#endif

/* MPI_COMM_TEST_INTER Fortran wrappers */
void mpi_comm_test_inter_(MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_comm_test_inter_(&comm_swap, flag, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_DIST_GRAPH_CREATE_ADJACENT Fortran wrappers */
void mpi_dist_graph_create_adjacent_(MPI_Fint *comm_old, MPI_Fint *indegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *outdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *comm_dist_graph, MPI_Fint *ierr)
{
    MPI_Fint comm_old_swap = geopm_swap_comm_world_f(*comm_old);
    pmpi_dist_graph_create_adjacent_(&comm_old_swap, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph, ierr);
}

/* MPI_DIST_GRAPH_CREATE Fortran wrappers */
void mpi_dist_graph_create_(MPI_Fint *comm_old, MPI_Fint *n, MPI_Fint *nodes, MPI_Fint *degrees, MPI_Fint *targets, MPI_Fint *weights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    MPI_Fint comm_old_swap = geopm_swap_comm_world_f(*comm_old);
    pmpi_dist_graph_create_(&comm_old_swap, n, nodes, degrees, targets, weights, info, reorder, newcomm, ierr);
}

/* MPI_DIST_GRAPH_NEIGHBORS_COUNT Fortran wrappers */
void mpi_dist_graph_neighbors_count_(MPI_Fint *comm, MPI_Fint *inneighbors, MPI_Fint *outneighbors, MPI_Fint *weighted, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_dist_graph_neighbors_count_(&comm_swap, inneighbors, outneighbors, weighted, ierr);
}

/* MPI_DIST_GRAPH_NEIGHBORS Fortran wrappers */
void mpi_dist_graph_neighbors_(MPI_Fint *comm, MPI_Fint *maxindegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *maxoutdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_dist_graph_neighbors(&comm_swap, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights, ierr);
}
#endif

/* MPI_EXSCAN Fortran wrappers */
void mpi_exscan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_exscan_(sendbuf, recvbuf, count, datatype, op, &comm_swap, ierr);
}

/* MPI_FILE_OPEN Fortran wrappers */
void mpi_file_open_(MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_Fint *fh, MPI_Fint *ierr, int name_len)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_file_open_(&comm_swap, filename, amode, info, fh, ierr, name_len);
}

/* MPI_FINALIZE Fortran wrappers */
void mpi_finalize_(MPI_Fint *ierr)
{
    *ierr = MPI_Finalize();
}

/* MPI_GATHER Fortran wrappers */
void mpi_gather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_gather_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_GATHERV Fortran wrappers */
void mpi_gatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_gatherv_(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_GRAPH_CREATE Fortran wrappers */
void mpi_graph_create_(MPI_Fint *comm_old, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *reorder, MPI_Fint *comm_graph, MPI_Fint *ierr)
{
    MPI_Fint comm_old_swap = geopm_swap_comm_world_f(*comm_old);
    pmpi_graph_create_(&comm_old_swap, nnodes, index, edges, reorder, comm_graph, ierr);
}

/* MPI_GRAPHDIMS_GET Fortran wrappers */
void mpi_graphdims_get_(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *nedges, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_graphdims_get(&comm_swap, nnodes, nedges, ierr);
}

/* MPI_GRAPH_GET Fortran wrappers */
void mpi_graph_get_(MPI_Fint *comm, MPI_Fint *maxindex, MPI_Fint *maxedges, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_graph_get(&comm_swap, maxindex, maxedges, index, edges, ierr);
}

/* MPI_GRAPH_MAP Fortran wrappers */
void mpi_graph_map_(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *newrank, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_graph_map_(&comm_swap, nnodes, index, edges, newrank, ierr);
}

/* MPI_GRAPH_NEIGHBORS_COUNT Fortran wrappers */
void mpi_graph_neighbors_count_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *nneighbors, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_graph_neighbors_count_(&comm_swap, rank, nneighbors, ierr);
}

/* MPI_GRAPH_NEIGHBORS Fortran wrappers */
void mpi_graph_neighbors_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxneighbors, MPI_Fint *neighbors, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_graph_neighbors_(&comm_swap, rank, maxneighbors, neighbors, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_IALLGATHER Fortran wrappers */
void mpi_iallgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iallgather_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, request, ierr);
}

/* MPI_IALLGATHERV Fortran wrappers */
void mpi_iallgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iallgatherv_(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, &comm_swap, request, ierr);
}

/* MPI_IALLREDUCE Fortran wrappers */
void mpi_iallreduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iallreduce_(sendbuf, recvbuf, count, datatype, op, &comm_swap, request, ierr);
}

/* MPI_IALLTOALL Fortran wrappers */
void mpi_ialltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ialltoall_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, request, ierr);
}

/* MPI_IALLTOALLV Fortran wrappers */
void mpi_ialltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ialltoallv_(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, &comm_swap, request, ierr);
}

/* MPI_IALLTOALLW Fortran wrappers */
void mpi_ialltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ialltoallw_(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, &comm_swap, request, ierr);
}

/* MPI_IBARRIER Fortran wrappers */
void mpi_ibarrier_(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ibarrier_(&comm_swap, request, ierr);
}

/* MPI_IBCAST Fortran wrappers */
void mpi_ibcast_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ibcast_(buf, count, datatype, root, &comm_swap, request, ierr);
}
#endif

/* MPI_IBSEND Fortran wrappers */
void mpi_ibsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ibsend_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_IEXSCAN Fortran wrappers */
void mpi_iexscan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iexscan_(sendbuf, recvbuf, count, datatype, op, &comm_swap, request, ierr);
}

/* MPI_IGATHER Fortran wrappers */
void mpi_igather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_igather_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, &comm_swap, request, ierr);
}

/* MPI_IGATHERV Fortran wrappers */
void mpi_igatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_igatherv_(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, &comm_swap, request, ierr);
}

/* MPI_IMPROBE Fortran wrappers */
void mpi_improbe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_improbe_(source, tag, &comm_swap, flag, message, status, ierr);
}
#endif

/* MPI_INEIGHBOR_ALLGATHER Fortran wrappers */
void mpi_ineighbor_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ineighbor_allgather_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, request, ierr);
}

/* MPI_INEIGHBOR_ALLGATHERV Fortran wrappers */
void mpi_ineighbor_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ineighbor_allgatherv_(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, &comm_swap, request, ierr);
}

/* MPI_INEIGHBOR_ALLTOALL Fortran wrappers */
void mpi_ineighbor_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ineighbor_alltoall_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, request, ierr);
}

/* MPI_INEIGHBOR_ALLTOALLV Fortran wrappers */
void mpi_ineighbor_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ineighbor_alltoallv_(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, &comm_swap, request, ierr);
}

/* MPI_INEIGHBOR_ALLTOALLW Fortran wrappers */
void mpi_ineighbor_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ineighbor_alltoallw_(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, &comm_swap, request, ierr);
}

/* MPI_INIT Fortran wrappers */
void mpi_init_(MPI_Fint *ierr)
{
    int argc = 0;
    char** argv = NULL;
    *ierr = MPI_Init(&argc, &argv);
}

/* MPI_INIT_THREAD Fortran wrappers */
void mpi_init_thread_(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr)
{
    int argc = 0;
    char** argv = NULL;
    *ierr = MPI_Init_thread(&argc, &argv, *required, provided);
}

/* MPI_INTERCOMM_CREATE Fortran wrappers */
void mpi_intercomm_create_(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *bridge_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    MPI_Fint bridge_comm_swap = geopm_swap_comm_world_f(*bridge_comm);
    MPI_Fint local_comm_swap = geopm_swap_comm_world_f(*local_comm);
    pmpi_intercomm_create_(&local_comm_swap, local_leader, &bridge_comm_swap, remote_leader, tag, newintercomm, ierr);
}

/* MPI_INTERCOMM_MERGE Fortran wrappers */
void mpi_intercomm_merge_(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    MPI_Fint intercomm_swap = geopm_swap_comm_world_f(*intercomm);
    pmpi_intercomm_merge_(&intercomm_swap, high, newintercomm, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_IPROBE Fortran wrappers */
void mpi_iprobe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iprobe_(source, tag, &comm_swap, flag, status, ierr);
}
#endif

/* MPI_IRECV Fortran wrappers */
void mpi_irecv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_irecv_(buf, count, datatype, source, tag, &comm_swap, request, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_IREDUCE Fortran wrappers */
void mpi_ireduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ireduce_(sendbuf, recvbuf, count, datatype, op, root, &comm_swap, request, ierr);
}

/* MPI_IREDUCE_SCATTER_BLOCK Fortran wrappers */
void mpi_ireduce_scatter_block_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ireduce_scatter_block_(sendbuf, recvbuf, recvcount, datatype, op, &comm_swap, request, ierr);
}

/* MPI_IREDUCE_SCATTER Fortran wrappers */
void mpi_ireduce_scatter_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ireduce_scatter_(sendbuf, recvbuf, recvcounts, datatype, op, &comm_swap, request, ierr);
}
#endif

/* MPI_IRSEND Fortran wrappers */
void mpi_irsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_irsend_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_ISCAN Fortran wrappers */
void mpi_iscan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iscan_(sendbuf, recvbuf, count, datatype, op, &comm_swap, request, ierr);
}

/* MPI_ISCATTER Fortran wrappers */
void mpi_iscatter_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iscatter_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, &comm_swap, request, ierr);
}

/* MPI_ISCATTERV Fortran wrappers */
void mpi_iscatterv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_iscatterv_(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, &comm_swap, request, ierr);
}
#endif

/* MPI_ISEND Fortran wrappers */
void mpi_isend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_isend_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
}

/* MPI_ISSEND Fortran wrappers */
void mpi_issend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_issend_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
}

/* MPI_MPROBE Fortran wrappers */
void mpi_mprobe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_mprobe_(source, tag, &comm_swap, message, status, ierr);
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_NEIGHBOR_ALLGATHER Fortran wrappers */
void mpi_neighbor_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_neighbor_allgather_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_NEIGHBOR_ALLGATHERV Fortran wrappers */
void mpi_neighbor_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_neighbor_allgatherv_(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_NEIGHBOR_ALLTOALL Fortran wrappers */
void mpi_neighbor_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_neighbor_alltoall_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_NEIGHBOR_ALLTOALLV Fortran wrappers */
void mpi_neighbor_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_neighbor_alltoallv_(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_NEIGHBOR_ALLTOALLW Fortran wrappers */
void mpi_neighbor_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_neighbor_alltoallw_(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}
#endif

/* MPI_PACK Fortran wrappers */
void mpi_pack_(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_pack_(inbuf, incount, datatype, outbuf, outsize, position, &comm_swap, ierr);
}

/* MPI_PACK_SIZE Fortran wrappers */
void mpi_pack_size_(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_pack_size_(incount, datatype, &comm_swap, size, ierr);
}

/* MPI_PROBE Fortran wrappers */
void mpi_probe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_probe_(source, tag, &comm_swap, status, ierr);
}

/* MPI_RECV_INIT Fortran wrappers */
void mpi_recv_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_recv_init_(buf, count, datatype, source, tag, &comm_swap, request, ierr);
}

/* MPI_RECV Fortran wrappers */
void mpi_recv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_recv_(buf, count, datatype, source, tag, &comm_swap, status, ierr);
}

/* MPI_REDUCE Fortran wrappers */
void mpi_reduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_reduce_(sendbuf, recvbuf, count, datatype, op, root, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_REDUCE_SCATTER_BLOCK Fortran wrappers */
void mpi_reduce_scatter_block_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_reduce_scatter_block_(sendbuf, recvbuf, recvcount, datatype, op, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}
#endif

/* MPI_REDUCE_SCATTER Fortran wrappers */
void mpi_reduce_scatter_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_reduce_scatter_(sendbuf, recvbuf, recvcounts, datatype, op, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_RSEND Fortran wrappers */
void mpi_rsend_(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_rsend_(ibuf, count, datatype, dest, tag, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_RSEND_INIT Fortran wrappers */
void mpi_rsend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_rsend_init_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_SCAN Fortran wrappers */
void mpi_scan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_scan_(sendbuf, recvbuf, count, datatype, op, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_SCATTER Fortran wrappers */
void mpi_scatter_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_scatter_(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_SCATTERV Fortran wrappers */
void mpi_scatterv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_scatterv_(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, &comm_swap, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_SEND Fortran wrappers */
void mpi_send_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_send_(buf, count, datatype, dest, tag, &comm_swap, ierr);
}

/* MPI_SEND_INIT Fortran wrappers */
void mpi_send_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_send_init_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
}

/* MPI_SENDRECV Fortran wrappers */
void mpi_sendrecv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_sendrecv_(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, &comm_swap, status, ierr);
}

/* MPI_SENDRECV_REPLACE Fortran wrappers */
void mpi_sendrecv_replace_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_sendrecv_replace_(buf, count, datatype, dest, sendtag, source, recvtag, &comm_swap, status, ierr);
}

/* MPI_SSEND Fortran wrappers */
void mpi_ssend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ssend_(buf, count, datatype, dest, tag, &comm_swap, ierr);
}

/* MPI_SSEND_INIT Fortran wrappers */
void mpi_ssend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_ssend_init_(buf, count, datatype, dest, tag, &comm_swap, request, ierr);
}

/* MPI_TOPO_TEST Fortran wrappers */
void mpi_topo_test_(MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_topo_test_(&comm_swap, status, ierr);
}

/* MPI_UNPACK Fortran wrappers */
void mpi_unpack_(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_unpack_(inbuf, insize, position, outbuf, outcount, datatype, &comm_swap, ierr);
}

/* MPI_WAITALL Fortran wrappers */
void mpi_waitall_(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_waitall_(count, array_of_requests, array_of_statuses, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_WAITANY Fortran wrappers */
void mpi_waitany_(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr)
{
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_waitany_(count, array_of_requests, index, status, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_WAIT Fortran wrappers */
void mpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_wait_(request, status, ierr);
    GEOPM_MPI_EXIT_MACRO
}

/* MPI_WAITSOME Fortran wrappers */
void mpi_waitsome_(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    GEOPM_MPI_ENTER_MACRO(__func__)
    pmpi_waitsome_(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, ierr);
    GEOPM_MPI_EXIT_MACRO
}

#ifdef GEOPM_ENABLE_MPI3
/* MPI_WIN_ALLOCATE Fortran wrappers */
void mpi_win_allocate_(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_win_allocate_(size, disp_unit, info, &comm_swap, baseptr, win, ierr);
}

/* MPI_WIN_ALLOCATE_SHARED Fortran wrappers */
void mpi_win_allocate_shared_(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_win_allocate_shared_(size, disp_unit, info, &comm_swap, baseptr, win, ierr);
}

/* MPI_WIN_CREATE_DYNAMIC Fortran wrappers */
void mpi_win_create_dynamic_(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_win_create_dynamic_(info, &comm_swap, win, ierr);
}
#endif

/* MPI_WIN_CREATE Fortran wrappers */
void mpi_win_create_(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    MPI_Fint comm_swap = geopm_swap_comm_world_f(*comm);
    pmpi_win_create_(base, size, disp_unit, info, &comm_swap, win, ierr);
}

/* Alternate Fortran symbols for MPI functions */
#ifdef GEOPM_ENABLE_MPI3
void (*mpi_comm_create_group)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_create_group_;
void (*mpi_comm_create_group__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_create_group_;
void (*MPI_COMM_CREATE_GROUP)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_create_group_;
void (*mpi_comm_get_info)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_info_;
void (*mpi_comm_get_info__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_info_;
void (*MPI_COMM_GET_INFO)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_info_;
void (*mpi_comm_idup)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_idup_;
void (*mpi_comm_idup__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_idup_;
void (*MPI_COMM_IDUP)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_idup_;
void (*mpi_comm_set_info)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_info_;
void (*mpi_comm_set_info__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_info_;
void (*MPI_COMM_SET_INFO)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_info_;
void (*mpi_comm_split_type)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_split_type_;
void (*mpi_comm_split_type__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_split_type_;
void (*MPI_COMM_SPLIT_TYPE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_split_type_;
void (*mpi_dist_graph_create_adjacent)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_create_adjacent_;
void (*mpi_dist_graph_create_adjacent__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_create_adjacent_;
void (*MPI_DIST_GRAPH_CREATE_ADJACENT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_create_adjacent_;
void (*mpi_dist_graph_create)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_create_;
void (*mpi_dist_graph_create__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_create_;
void (*MPI_DIST_GRAPH_CREATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_create_;
void (*mpi_dist_graph_neighbors_count)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_neighbors_count_;
void (*mpi_dist_graph_neighbors_count__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_neighbors_count_;
void (*MPI_DIST_GRAPH_NEIGHBORS_COUNT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_neighbors_count_;
void (*mpi_dist_graph_neighbors)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_neighbors_;
void (*mpi_dist_graph_neighbors__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_neighbors_;
void (*MPI_DIST_GRAPH_NEIGHBORS)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_dist_graph_neighbors_;
void (*mpi_iallgather)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallgather_;
void (*mpi_iallgather__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallgather_;
void (*MPI_IALLGATHER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallgather_;
void (*mpi_iallgatherv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallgatherv_;
void (*mpi_iallgatherv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallgatherv_;
void (*MPI_IALLGATHERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallgatherv_;
void (*mpi_iallreduce)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallreduce_;
void (*mpi_iallreduce__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallreduce_;
void (*MPI_IALLREDUCE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iallreduce_;
void (*mpi_ialltoall)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoall_;
void (*mpi_ialltoall__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoall_;
void (*MPI_IALLTOALL)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoall_;
void (*mpi_ialltoallv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoallv_;
void (*mpi_ialltoallv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoallv_;
void (*MPI_IALLTOALLV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoallv_;
void (*mpi_ialltoallw)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoallw_;
void (*mpi_ialltoallw__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoallw_;
void (*MPI_IALLTOALLW)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ialltoallw_;
void (*mpi_ibarrier)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibarrier_;
void (*mpi_ibarrier__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibarrier_;
void (*MPI_IBARRIER)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibarrier_;
void (*mpi_ibcast)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibcast_;
void (*mpi_ibcast__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibcast_;
void (*MPI_IBCAST)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibcast_;
void (*mpi_iexscan)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iexscan_;
void (*mpi_iexscan__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iexscan_;
void (*MPI_IEXSCAN)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iexscan_;
void (*mpi_igather)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_igather_;
void (*mpi_igather__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_igather_;
void (*MPI_IGATHER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_igather_;
void (*mpi_igatherv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_igatherv_;
void (*mpi_igatherv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_igatherv_;
void (*MPI_IGATHERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_igatherv_;
void (*mpi_improbe)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_improbe_;
void (*mpi_improbe__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_improbe_;
void (*MPI_IMPROBE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_improbe_;
void (*mpi_iprobe)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iprobe_;
void (*mpi_iprobe__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iprobe_;
void (*MPI_IPROBE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iprobe_;
void (*mpi_ireduce)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_;
void (*mpi_ireduce__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_;
void (*MPI_IREDUCE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_;
void (*mpi_ireduce_scatter_block)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_scatter_block_;
void (*mpi_ireduce_scatter_block__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_scatter_block_;
void (*MPI_IREDUCE_SCATTER_BLOCK)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_scatter_block_;
void (*mpi_ireduce_scatter)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_scatter_;
void (*mpi_ireduce_scatter__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_scatter_;
void (*MPI_IREDUCE_SCATTER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ireduce_scatter_;
void (*mpi_iscan)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscan_;
void (*mpi_iscan__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscan_;
void (*MPI_ISCAN)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscan_;
void (*mpi_iscatter)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscatter_;
void (*mpi_iscatter__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscatter_;
void (*MPI_ISCATTER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscatter_;
void (*mpi_iscatterv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscatterv_;
void (*mpi_iscatterv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscatterv_;
void (*MPI_ISCATTERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_iscatterv_;
void (*mpi_neighbor_allgather)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_allgather_;
void (*mpi_neighbor_allgather__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_allgather_;
void (*MPI_NEIGHBOR_ALLGATHER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_allgather_;
void (*mpi_neighbor_allgatherv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_allgatherv_;
void (*mpi_neighbor_allgatherv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_allgatherv_;
void (*MPI_NEIGHBOR_ALLGATHERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_allgatherv_;
void (*mpi_neighbor_alltoall)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoall_;
void (*mpi_neighbor_alltoall__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoall_;
void (*MPI_NEIGHBOR_ALLTOALL)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoall_;
void (*mpi_neighbor_alltoallv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoallv_;
void (*mpi_neighbor_alltoallv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoallv_;
void (*MPI_NEIGHBOR_ALLTOALLV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoallv_;
void (*mpi_neighbor_alltoallw)(MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoallw_;
void (*mpi_neighbor_alltoallw__)(MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoallw_;
void (*MPI_NEIGHBOR_ALLTOALLW)(MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_neighbor_alltoallw_;
void (*mpi_reduce_scatter_block)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_scatter_block_;
void (*mpi_reduce_scatter_block__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_scatter_block_;
void (*MPI_REDUCE_SCATTER_BLOCK)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_scatter_block_;
void (*mpi_win_allocate)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_allocate_;
void (*mpi_win_allocate__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_allocate_;
void (*MPI_WIN_ALLOCATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_allocate_;
void (*mpi_win_allocate_shared)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_allocate_shared_;
void (*mpi_win_allocate_shared__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_allocate_shared_;
void (*MPI_WIN_ALLOCATE_SHARED)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_allocate_shared_;
void (*mpi_win_create_dynamic)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_create_dynamic_;
void (*mpi_win_create_dynamic__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_create_dynamic_;
void (*MPI_WIN_CREATE_DYNAMIC)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_create_dynamic_;
#endif
void (*mpi_allgather)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allgather_;
void (*mpi_allgather__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allgather_;
void (*MPI_ALLGATHER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allgather_;
void (*mpi_allgatherv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allgatherv_;
void (*mpi_allgatherv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allgatherv_;
void (*MPI_ALLGATHERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allgatherv_;
void (*mpi_allreduce)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allreduce_;
void (*mpi_allreduce__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allreduce_;
void (*MPI_ALLREDUCE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_allreduce_;
void (*mpi_alltoall)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoall_;
void (*mpi_alltoall__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoall_;
void (*MPI_ALLTOALL)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoall_;
void (*mpi_alltoallv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoallv_;
void (*mpi_alltoallv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoallv_;
void (*MPI_ALLTOALLV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoallv_;
void (*mpi_alltoallw)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoallw_;
void (*mpi_alltoallw__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoallw_;
void (*MPI_ALLTOALLW)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_alltoallw_;
void (*mpi_barrier)(MPI_Fint*,MPI_Fint*) = &mpi_barrier_;
void (*mpi_barrier__)(MPI_Fint*,MPI_Fint*) = &mpi_barrier_;
void (*MPI_BARRIER)(MPI_Fint*,MPI_Fint*) = &mpi_barrier_;
void (*mpi_bcast)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bcast_;
void (*mpi_bcast__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bcast_;
void (*MPI_BCAST)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bcast_;
void (*mpi_bsend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bsend_;
void (*mpi_bsend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bsend_;
void (*MPI_BSEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bsend_;
void (*mpi_bsend_init)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bsend_init_;
void (*mpi_bsend_init__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bsend_init_;
void (*MPI_BSEND_INIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_bsend_init_;
void (*mpi_cart_coords)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_coords_;
void (*mpi_cart_coords__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_coords_;
void (*MPI_CART_COORDS)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_coords_;
void (*mpi_cart_create)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_create_;
void (*mpi_cart_create__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_create_;
void (*MPI_CART_CREATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_create_;
void (*mpi_cartdim_get)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cartdim_get_;
void (*mpi_cartdim_get__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cartdim_get_;
void (*MPI_CARTDIM_GET)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cartdim_get_;
void (*mpi_cart_get)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_get_;
void (*mpi_cart_get__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_get_;
void (*MPI_CART_GET)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_get_;
void (*mpi_cart_map)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_map_;
void (*mpi_cart_map__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_map_;
void (*MPI_CART_MAP)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_map_;
void (*mpi_cart_rank)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_rank_;
void (*mpi_cart_rank__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_rank_;
void (*MPI_CART_RANK)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_rank_;
void (*mpi_cart_shift)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_shift_;
void (*mpi_cart_shift__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_shift_;
void (*MPI_CART_SHIFT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_shift_;
void (*mpi_cart_sub)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_sub_;
void (*mpi_cart_sub__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_sub_;
void (*MPI_CART_SUB)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_cart_sub_;
void (*mpi_comm_accept)(char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_accept_;
void (*mpi_comm_accept__)(char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_accept_;
void (*MPI_COMM_ACCEPT)(char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_accept_;
void (*mpi_comm_call_errhandler)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_call_errhandler_;
void (*mpi_comm_call_errhandler__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_call_errhandler_;
void (*MPI_COMM_CALL_ERRHANDLER)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_call_errhandler_;
void (*mpi_comm_compare)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_compare_;
void (*mpi_comm_compare__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_compare_;
void (*MPI_COMM_COMPARE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_compare_;
void (*mpi_comm_connect)(char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_connect_;
void (*mpi_comm_connect__)(char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_connect_;
void (*MPI_COMM_CONNECT)(char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_connect_;
void (*mpi_comm_create)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_create_;
void (*mpi_comm_create__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_create_;
void (*MPI_COMM_CREATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_create_;
void (*mpi_comm_delete_attr)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_delete_attr_;
void (*mpi_comm_delete_attr__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_delete_attr_;
void (*MPI_COMM_DELETE_ATTR)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_delete_attr_;
void (*mpi_comm_dup)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_dup_;
void (*mpi_comm_dup__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_dup_;
void (*MPI_COMM_DUP)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_dup_;
void (*mpi_comm_dup_with_info)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_dup_with_info_;
void (*mpi_comm_dup_with_info__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_dup_with_info_;
void (*MPI_COMM_DUP_WITH_INFO)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_dup_with_info_;
void (*mpi_comm_get_attr)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_attr_;
void (*mpi_comm_get_attr__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_attr_;
void (*MPI_COMM_GET_ATTR)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_attr_;
void (*mpi_comm_get_errhandler)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_errhandler_;
void (*mpi_comm_get_errhandler__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_errhandler_;
void (*MPI_COMM_GET_ERRHANDLER)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_get_errhandler_;
void (*mpi_comm_get_name)(MPI_Fint*,char*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_get_name_;
void (*mpi_comm_get_name__)(MPI_Fint*,char*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_get_name_;
void (*MPI_COMM_GET_NAME)(MPI_Fint*,char*,MPI_Fint*,MPI_Fint*,int) = &mpi_comm_get_name_;
void (*mpi_comm_get_parent)(MPI_Fint*,MPI_Fint*) = &mpi_comm_get_parent_;
void (*mpi_comm_get_parent__)(MPI_Fint*,MPI_Fint*) = &mpi_comm_get_parent_;
void (*MPI_COMM_GET_PARENT)(MPI_Fint*,MPI_Fint*) = &mpi_comm_get_parent_;
void (*mpi_comm_group)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_group_;
void (*mpi_comm_group__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_group_;
void (*MPI_COMM_GROUP)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_group_;
void (*mpi_comm_rank)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_rank_;
void (*mpi_comm_rank__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_rank_;
void (*MPI_COMM_RANK)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_rank_;
void (*mpi_comm_remote_group)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_remote_group_;
void (*mpi_comm_remote_group__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_remote_group_;
void (*MPI_COMM_REMOTE_GROUP)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_remote_group_;
void (*mpi_comm_remote_size)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_remote_size_;
void (*mpi_comm_remote_size__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_remote_size_;
void (*MPI_COMM_REMOTE_SIZE)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_remote_size_;
void (*mpi_comm_set_attr)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_attr_;
void (*mpi_comm_set_attr__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_attr_;
void (*MPI_COMM_SET_ATTR)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_attr_;
void (*mpi_comm_set_errhandler)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_errhandler_;
void (*mpi_comm_set_errhandler__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_errhandler_;
void (*MPI_COMM_SET_ERRHANDLER)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_set_errhandler_;
void (*mpi_comm_set_name)(MPI_Fint*,char*,MPI_Fint*,int) = &mpi_comm_set_name_;
void (*mpi_comm_set_name__)(MPI_Fint*,char*,MPI_Fint*,int) = &mpi_comm_set_name_;
void (*MPI_COMM_SET_NAME)(MPI_Fint*,char*,MPI_Fint*,int) = &mpi_comm_set_name_;
void (*mpi_comm_size)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_size_;
void (*mpi_comm_size__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_size_;
void (*MPI_COMM_SIZE)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_size_;
void (*mpi_comm_spawn)(char*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int,int) = &mpi_comm_spawn_;
void (*mpi_comm_spawn__)(char*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int,int) = &mpi_comm_spawn_;
void (*MPI_COMM_SPAWN)(char*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int,int) = &mpi_comm_spawn_;
void (*mpi_comm_spawn_multiple)(MPI_Fint*,char*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int,int) = &mpi_comm_spawn_multiple_;
void (*mpi_comm_spawn_multiple__)(MPI_Fint*,char*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int,int) = &mpi_comm_spawn_multiple_;
void (*MPI_COMM_SPAWN_MULTIPLE)(MPI_Fint*,char*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int,int) = &mpi_comm_spawn_multiple_;
void (*mpi_comm_split)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_split_;
void (*mpi_comm_split__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_split_;
void (*MPI_COMM_SPLIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_split_;
void (*mpi_comm_test_inter)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_test_inter_;
void (*mpi_comm_test_inter__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_test_inter_;
void (*MPI_COMM_TEST_INTER)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_comm_test_inter_;
void (*mpi_exscan)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_exscan_;
void (*mpi_exscan__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_exscan_;
void (*MPI_EXSCAN)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_exscan_;
void (*mpi_file_open)(MPI_Fint*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_file_open_;
void (*mpi_file_open__)(MPI_Fint*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_file_open_;
void (*MPI_FILE_OPEN)(MPI_Fint*,char*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,int) = &mpi_file_open_;
void (*mpi_finalize)(MPI_Fint*) = &mpi_finalize_;
void (*mpi_finalize__)(MPI_Fint*) = &mpi_finalize_;
void (*MPI_FINALIZE)(MPI_Fint*) = &mpi_finalize_;
void (*mpi_gatherv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_gatherv_;
void (*mpi_gatherv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_gatherv_;
void (*MPI_GATHERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_gatherv_;
void (*mpi_graph_create)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_create_;
void (*mpi_graph_create__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_create_;
void (*MPI_GRAPH_CREATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_create_;
void (*mpi_graphdims_get)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graphdims_get_;
void (*mpi_graphdims_get__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graphdims_get_;
void (*MPI_GRAPHDIMS_GET)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graphdims_get_;
void (*mpi_graph_get)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_get_;
void (*mpi_graph_get__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_get_;
void (*MPI_GRAPH_GET)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_get_;
void (*mpi_graph_map)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_map_;
void (*mpi_graph_map__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_map_;
void (*MPI_GRAPH_MAP)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_map_;
void (*mpi_graph_neighbors_count)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_neighbors_count_;
void (*mpi_graph_neighbors_count__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_neighbors_count_;
void (*MPI_GRAPH_NEIGHBORS_COUNT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_neighbors_count_;
void (*mpi_graph_neighbors)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_neighbors_;
void (*mpi_graph_neighbors__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_neighbors_;
void (*MPI_GRAPH_NEIGHBORS)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_graph_neighbors_;
void (*mpi_ibsend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibsend_;
void (*mpi_ibsend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibsend_;
void (*MPI_IBSEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ibsend_;
void (*mpi_ineighbor_allgather)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_allgather_;
void (*mpi_ineighbor_allgather__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_allgather_;
void (*MPI_INEIGHBOR_ALLGATHER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_allgather_;
void (*mpi_ineighbor_allgatherv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_allgatherv_;
void (*mpi_ineighbor_allgatherv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_allgatherv_;
void (*MPI_INEIGHBOR_ALLGATHERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_allgatherv_;
void (*mpi_ineighbor_alltoall)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoall_;
void (*mpi_ineighbor_alltoall__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoall_;
void (*MPI_INEIGHBOR_ALLTOALL)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoall_;
void (*mpi_ineighbor_alltoallv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoallv_;
void (*mpi_ineighbor_alltoallv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoallv_;
void (*MPI_INEIGHBOR_ALLTOALLV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoallv_;
void (*mpi_ineighbor_alltoallw)(MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoallw_;
void (*mpi_ineighbor_alltoallw__)(MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoallw_;
void (*MPI_INEIGHBOR_ALLTOALLW)(MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Aint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ineighbor_alltoallw_;
void (*mpi_init)(MPI_Fint*) = &mpi_init_;
void (*mpi_init__)(MPI_Fint*) = &mpi_init_;
void (*MPI_INIT)(MPI_Fint*) = &mpi_init_;
void (*mpi_intercomm_create)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_intercomm_create_;
void (*mpi_intercomm_create__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_intercomm_create_;
void (*MPI_INTERCOMM_CREATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_intercomm_create_;
void (*mpi_intercomm_merge)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_intercomm_merge_;
void (*mpi_intercomm_merge__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_intercomm_merge_;
void (*MPI_INTERCOMM_MERGE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_intercomm_merge_;
void (*mpi_irecv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_irecv_;
void (*mpi_irecv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_irecv_;
void (*MPI_IRECV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_irecv_;
void (*mpi_irsend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_irsend_;
void (*mpi_irsend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_irsend_;
void (*MPI_IRSEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_irsend_;
void (*mpi_isend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_isend_;
void (*mpi_isend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_isend_;
void (*MPI_ISEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_isend_;
void (*mpi_issend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_issend_;
void (*mpi_issend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_issend_;
void (*MPI_ISSEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_issend_;
void (*mpi_mprobe)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_mprobe_;
void (*mpi_mprobe__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_mprobe_;
void (*MPI_MPROBE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_mprobe_;
void (*mpi_pack)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_pack_;
void (*mpi_pack__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_pack_;
void (*MPI_PACK)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_pack_;
void (*mpi_pack_size)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_pack_size_;
void (*mpi_pack_size__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_pack_size_;
void (*MPI_PACK_SIZE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_pack_size_;
void (*mpi_probe)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_probe_;
void (*mpi_probe__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_probe_;
void (*MPI_PROBE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_probe_;
void (*mpi_recv_init)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_recv_init_;
void (*mpi_recv_init__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_recv_init_;
void (*MPI_RECV_INIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_recv_init_;
void (*mpi_recv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_recv_;
void (*mpi_recv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_recv_;
void (*MPI_RECV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_recv_;
void (*mpi_reduce)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_;
void (*mpi_reduce__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_;
void (*MPI_REDUCE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_;
void (*mpi_reduce_scatter)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_scatter_;
void (*mpi_reduce_scatter__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_scatter_;
void (*MPI_REDUCE_SCATTER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_reduce_scatter_;
void (*mpi_rsend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_rsend_;
void (*mpi_rsend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_rsend_;
void (*MPI_RSEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_rsend_;
void (*mpi_rsend_init)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_rsend_init_;
void (*mpi_rsend_init__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_rsend_init_;
void (*MPI_RSEND_INIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_rsend_init_;
void (*mpi_scan)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scan_;
void (*mpi_scan__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scan_;
void (*MPI_SCAN)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scan_;
void (*mpi_scatter)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scatter_;
void (*mpi_scatter__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scatter_;
void (*MPI_SCATTER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scatter_;
void (*mpi_scatterv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scatterv_;
void (*mpi_scatterv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scatterv_;
void (*MPI_SCATTERV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_scatterv_;
void (*mpi_send)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_send_;
void (*mpi_send__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_send_;
void (*MPI_SEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_send_;
void (*mpi_send_init)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_send_init_;
void (*mpi_send_init__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_send_init_;
void (*MPI_SEND_INIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_send_init_;
void (*mpi_sendrecv)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_sendrecv_;
void (*mpi_sendrecv__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_sendrecv_;
void (*MPI_SENDRECV)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_sendrecv_;
void (*mpi_sendrecv_replace)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_sendrecv_replace_;
void (*mpi_sendrecv_replace__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_sendrecv_replace_;
void (*MPI_SENDRECV_REPLACE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_sendrecv_replace_;
void (*mpi_ssend)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ssend_;
void (*mpi_ssend__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ssend_;
void (*MPI_SSEND)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ssend_;
void (*mpi_ssend_init)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ssend_init_;
void (*mpi_ssend_init__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ssend_init_;
void (*MPI_SSEND_INIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_ssend_init_;
void (*mpi_topo_test)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_topo_test_;
void (*mpi_topo_test__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_topo_test_;
void (*MPI_TOPO_TEST)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_topo_test_;
void (*mpi_unpack)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_unpack_;
void (*mpi_unpack__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_unpack_;
void (*MPI_UNPACK)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_unpack_;
void (*mpi_waitall)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitall_;
void (*mpi_waitall__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitall_;
void (*MPI_WAITALL)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitall_;
void (*mpi_waitany)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitany_;
void (*mpi_waitany__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitany_;
void (*MPI_WAITANY)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitany_;
void (*mpi_wait)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_wait_;
void (*mpi_wait__)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_wait_;
void (*MPI_WAIT)(MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_wait_;
void (*mpi_waitsome)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitsome_;
void (*mpi_waitsome__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitsome_;
void (*MPI_WAITSOME)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_waitsome_;
void (*mpi_win_create)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_create_;
void (*mpi_win_create__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_create_;
void (*MPI_WIN_CREATE)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_win_create_;
void (*mpi_gather)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_gather_;
void (*mpi_gather__)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_gather_;
void (*MPI_GATHER)(MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*,MPI_Fint*) = &mpi_gather_;
