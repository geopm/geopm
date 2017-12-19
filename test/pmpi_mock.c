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

int PMPI_Allgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Allgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Allreduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Alltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Alltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Alltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Barrier(MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Bsend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Bsend_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Gather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Gatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Neighbor_allgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Neighbor_allgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Neighbor_alltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Neighbor_alltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Neighbor_alltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST MPI_Aint sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST MPI_Aint rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Reduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Reduce_scatter(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Reduce_scatter_block(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Rsend(GEOPM_MPI_CONST void *ibuf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Rsend_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Scan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Scatter(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Scatterv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iallgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iallgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iallreduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ialltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ialltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ialltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cart_create(MPI_Comm old_comm, int ndims, GEOPM_MPI_CONST int dims[], GEOPM_MPI_CONST int periods[], int reorder, MPI_Comm *comm_cart)
{
    g_passed_comm_arg = old_comm;
    return 0;
}
int PMPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cart_map(MPI_Comm comm, int ndims, GEOPM_MPI_CONST int dims[], GEOPM_MPI_CONST int periods[], int *newrank)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cart_rank(MPI_Comm comm, GEOPM_MPI_CONST int coords[], int *rank)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cart_sub(MPI_Comm comm, GEOPM_MPI_CONST int remain_dims[], MPI_Comm *new_comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_accept(GEOPM_MPI_CONST char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    g_passed_comm_arg = comm1;
    if (comm1 != comm2) {
        return 1;
    }
    return 0;
}
int PMPI_Comm_connect(GEOPM_MPI_CONST char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Dist_graph_create(MPI_Comm comm_old, int n, GEOPM_MPI_CONST int nodes[], GEOPM_MPI_CONST int degrees[], GEOPM_MPI_CONST int targets[], GEOPM_MPI_CONST int weights[], MPI_Info info, int reorder, MPI_Comm * newcomm)
{
    g_passed_comm_arg = comm_old;
    return 0;
}
int PMPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, GEOPM_MPI_CONST int sources[], GEOPM_MPI_CONST int sourceweights[], int outdegree, GEOPM_MPI_CONST int destinations[], GEOPM_MPI_CONST int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
{
    g_passed_comm_arg = comm_old;
    return 0;
}
int PMPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Dist_graph_neighbors_count(MPI_Comm comm, int *inneighbors, int *outneighbors, int *weighted)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *erhandler)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_rank(MPI_Comm comm, int *rank)
{
*rank = 0;
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_remote_size(MPI_Comm comm, int *size)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_set_name(MPI_Comm comm, GEOPM_MPI_CONST char *comm_name)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_size(MPI_Comm comm, int *size)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_spawn(GEOPM_MPI_CONST char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], GEOPM_MPI_CONST int array_of_maxprocs[], GEOPM_MPI_CONST MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Exscan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iexscan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_File_open(MPI_Comm comm, GEOPM_MPI_CONST char *filename, int amode, MPI_Info info, MPI_File *fh)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Igather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Igatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, GEOPM_MPI_CONST int index[], GEOPM_MPI_CONST int edges[], int reorder, MPI_Comm *comm_graph)
{
    g_passed_comm_arg = comm_old;
    return 0;
}
int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Graph_map(MPI_Comm comm, int nnodes, GEOPM_MPI_CONST int index[], GEOPM_MPI_CONST int edges[], int *newrank)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[])
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ibsend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm bridge_comm, int remote_leader, int tag, MPI_Comm *newintercomm)
{
    g_passed_comm_arg = local_comm;
    return 0;
}
int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintercomm)
{
    g_passed_comm_arg = intercomm;
    return 0;
}
int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Irsend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Isend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Issend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ineighbor_allgather(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ineighbor_allgatherv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ineighbor_alltoall(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ineighbor_alltoallv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int sdispls[], MPI_Datatype sendtype, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ineighbor_alltoallw(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST MPI_Aint sdispls[], GEOPM_MPI_CONST MPI_Datatype sendtypes[], void *recvbuf, GEOPM_MPI_CONST int recvcounts[], GEOPM_MPI_CONST MPI_Aint rdispls[], GEOPM_MPI_CONST MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Pack(GEOPM_MPI_CONST void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ireduce(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ireduce_scatter(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, GEOPM_MPI_CONST int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ireduce_scatter_block(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iscan(GEOPM_MPI_CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iscatter(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Iscatterv(GEOPM_MPI_CONST void *sendbuf, GEOPM_MPI_CONST int sendcounts[], GEOPM_MPI_CONST int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Send_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Send(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Sendrecv(GEOPM_MPI_CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Sendrecv_replace(void * buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ssend_init(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Ssend(GEOPM_MPI_CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Topo_test(MPI_Comm comm, int *status)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Unpack(GEOPM_MPI_CONST void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    g_passed_comm_arg = comm;
    return 0;
}
int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    g_passed_comm_arg = comm;
    return 0;
}
