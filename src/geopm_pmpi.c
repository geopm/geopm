#include <stdlib.h>
#include <mpi.h>

#include "geopm.h"

static MPI_Comm GEOPM_SPLIT_MPI_COMM;

static MPI_Comm geopm_swap_comm_world(MPI_Comm comm)
{
    int is_comm_world = 0;
    (void)PMPI_Comm_compare(MPI_COMM_WORLD, comm, &is_comm_world);
    if (is_comm_world) {
        comm = GEOPM_SPLIT_MPI_COMM;
    }
    return comm;
}

int MPI_Init(int *argc, char **argv[])
{
    int is_ctl;
    int err = PMPI_Init(argc, argv);

    if (!err) {
        err = geopm_comm_split(MPI_COMM_WORLD, &GEOPM_SPLIT_MPI_COMM, &is_ctl);
    }
    if (!err && is_ctl) {
        struct geopm_policy_c *policy;
        struct geopm_ctl_c *ctl;
        err = geopm_policy_create(getenv("GEOPM_POLICY"), NULL, &policy);
        if (!err) {
            err = geopm_ctl_create(policy, getenv("GEOPM_SHMKEY"), GEOPM_SPLIT_MPI_COMM, &ctl);
        }
        if (!err) {
            err = geopm_ctl_run(ctl);
        }
        MPI_Finalize();
        exit(err);
    }
    return err;
}

int MPI_Finalize(void)
{
    MPI_Comm_free(&GEOPM_SPLIT_MPI_COMM);
    return PMPI_Finalize();
}

int MPI_Abort(MPI_Comm comm, int errorcode)
{
    return PMPI_Abort(geopm_swap_comm_world(comm), errorcode);
}

int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount,
                   MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iallgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, const int recvcounts[],
                   const int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int recvcounts[],
                    const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iallgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count,
                  MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return PMPI_Allreduce(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm));
}

int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count,
                   MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iallreduce(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount,
                  MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                  MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                  const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Alltoallv(sendbuf, sendcounts,sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[],
                   MPI_Datatype sendtype, void *recvbuf, const int recvcounts[],
                   const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ialltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[],
                  void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[],
                  MPI_Comm comm)
{
    return PMPI_Alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm));
}

int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[],
                   void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[],
                   MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ialltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm), request);
}

int MPI_Barrier(MPI_Comm comm)
{
    return PMPI_Barrier(geopm_swap_comm_world(comm));
}

int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ibarrier(geopm_swap_comm_world(comm), request);
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    return PMPI_Bcast(buffer, count, datatype, root, geopm_swap_comm_world(comm));
}

int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    return PMPI_Bsend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
}

int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ibcast(buffer, count, datatype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Bsend_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[])
{
    return PMPI_Cart_coords(geopm_swap_comm_world(comm), rank, maxdims, coords);
}

int MPI_Cart_create(MPI_Comm old_comm, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart)
{
    return PMPI_Cart_create(geopm_swap_comm_world(old_comm), ndims, dims, periods, reorder, comm_cart);
}

int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[])
{
    return PMPI_Cart_get(geopm_swap_comm_world(comm), maxdims, dims, periods, coords);
}

int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank)
{
    return PMPI_Cart_map(geopm_swap_comm_world(comm), ndims, dims, periods, newrank);
}

int MPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank)
{
    return PMPI_Cart_rank(geopm_swap_comm_world(comm), coords, rank);
}

int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest)
{
    return PMPI_Cart_shift(geopm_swap_comm_world(comm), direction, disp, rank_source, rank_dest);
}

int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *new_comm)
{
    return PMPI_Cart_sub(geopm_swap_comm_world(comm), remain_dims, new_comm);
}

int MPI_Cartdim_get(MPI_Comm comm, int *ndims)
{
    return PMPI_Cartdim_get(geopm_swap_comm_world(comm), ndims);
}

int MPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    return PMPI_Comm_accept(port_name, info, root, geopm_swap_comm_world(comm), newcomm);
}

MPI_Fint MPI_Comm_c2f(MPI_Comm comm)
{
    return PMPI_Comm_c2f(geopm_swap_comm_world(comm));
}

int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode)
{
    return PMPI_Comm_call_errhandler(geopm_swap_comm_world(comm), errorcode);
}

int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result)
{
    return PMPI_Comm_compare(geopm_swap_comm_world(comm1), geopm_swap_comm_world(comm2), result);
}

int MPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm)
{
    return PMPI_Comm_connect(port_name, info, root, geopm_swap_comm_world(comm), newcomm);
}

int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm)
{
    return PMPI_Comm_create_group(geopm_swap_comm_world(comm), group, tag, newcomm);
}

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

int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request)
{
    return PMPI_Comm_idup(geopm_swap_comm_world(comm), newcomm, request);
}

int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm)
{
    return PMPI_Comm_dup_with_info(geopm_swap_comm_world(comm), info, newcomm);
}

MPI_Comm MPI_Comm_f2c(MPI_Fint comm)
{
    return geopm_swap_comm_world(PMPI_Comm_f2c(comm));
}

int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag)
{
    return PMPI_Comm_get_attr(geopm_swap_comm_world(comm), comm_keyval, attribute_val, flag);
}

int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int nodes[], const int degrees[], const int targets[], const int weights[], MPI_Info info, int reorder, MPI_Comm * newcomm)
{
    return PMPI_Dist_graph_create(geopm_swap_comm_world(comm_old), n, nodes, degrees, targets, weights, info, reorder, newcomm);
}

int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph)
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

int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *erhandler)
{
    return PMPI_Comm_get_errhandler(geopm_swap_comm_world(comm), erhandler);
}

int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used)
{
    return PMPI_Comm_get_info(geopm_swap_comm_world(comm), info_used);
}

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

int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info)
{
    return PMPI_Comm_set_info(geopm_swap_comm_world(comm), info);
}

int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name)
{
    return PMPI_Comm_set_name(geopm_swap_comm_world(comm), comm_name);
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
    return PMPI_Comm_size(geopm_swap_comm_world(comm), size);
}

int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    return PMPI_Comm_spawn(command, argv, maxprocs, info, root, geopm_swap_comm_world(comm), intercomm, array_of_errcodes);
}

int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[])
{
    return PMPI_Comm_spawn_multiple(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, geopm_swap_comm_world(comm), intercomm, array_of_errcodes);
}

int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm)
{
    return PMPI_Comm_split(geopm_swap_comm_world(comm), color, key, newcomm);
}

int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm)
{
    return PMPI_Comm_split_type(geopm_swap_comm_world(comm), split_type, key, info, newcomm);
}

int MPI_Comm_test_inter(MPI_Comm comm, int *flag)
{
    return PMPI_Comm_test_inter(geopm_swap_comm_world(comm), flag);
}

int MPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return PMPI_Exscan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm));
}

int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iexscan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh)
{
    return PMPI_File_open(geopm_swap_comm_world(comm), filename, amode, info, fh);
}

int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return PMPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm));
}

int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return PMPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, geopm_swap_comm_world(comm));
}

int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Igatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Graph_create(MPI_Comm comm_old, int nnodes, const int index[], const int edges[], int reorder, MPI_Comm *comm_graph)
{
    return PMPI_Graph_create(geopm_swap_comm_world(comm_old), nnodes, index, edges, reorder, comm_graph);
}

int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int index[], int edges[])
{
    return PMPI_Graph_get(geopm_swap_comm_world(comm), maxindex, maxedges, index, edges);
}

int MPI_Graph_map(MPI_Comm comm, int nnodes, const int index[], const int edges[], int *newrank)
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

int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ibsend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status)
{
    return PMPI_Improbe(source, tag, geopm_swap_comm_world(comm), flag, message, status);
}

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

int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Irsend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Isend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Issend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status)
{
    return PMPI_Mprobe(source, tag, geopm_swap_comm_world(comm), message, status);
}

int MPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Neighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Neighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Neighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm)
{
    return PMPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm));
}

int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, geopm_swap_comm_world(comm), request);
}

int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm)
{
    return PMPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm));
}

int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, geopm_swap_comm_world(comm), request);
}

int MPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm)
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
    return PMPI_Recv(buf, count, datatype, source, tag, geopm_swap_comm_world(comm), status);
}

int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
{
    return PMPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, geopm_swap_comm_world(comm));
}

int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, geopm_swap_comm_world(comm), request);
}

int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return PMPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, geopm_swap_comm_world(comm));
}

int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return PMPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, geopm_swap_comm_world(comm));
}

int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ireduce_scatter_block(sendbuf, recvbuf, recvcount, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Rsend(const void *ibuf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    return PMPI_Rsend(ibuf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
}

int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Rsend_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm)
{
    return PMPI_Scan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm));
}

int MPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iscan(sendbuf, recvbuf, count, datatype, op, geopm_swap_comm_world(comm), request);
}

int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return PMPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm));
}

int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
    return PMPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm));
}

int MPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Iscatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, geopm_swap_comm_world(comm), request);
}

int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Send_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    return PMPI_Send(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
}

int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    return PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, geopm_swap_comm_world(comm), status);
}

int MPI_Sendrecv_replace(void * buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status)
{
    return PMPI_Sendrecv_replace(buf, count, datatype, dest, sendtag, source, recvtag, geopm_swap_comm_world(comm), status);
}

int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
    return PMPI_Ssend_init(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm), request);
}

int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    return PMPI_Ssend(buf, count, datatype, dest, tag, geopm_swap_comm_world(comm));
}

int MPI_Topo_test(MPI_Comm comm, int *status)
{
    return PMPI_Topo_test(geopm_swap_comm_world(comm), status);
}

int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
    return PMPI_Unpack(inbuf, insize, position, outbuf, outcount, datatype, geopm_swap_comm_world(comm));
}

int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    return PMPI_Win_allocate(size, disp_unit, info, geopm_swap_comm_world(comm), baseptr, win);
}

int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win)
{
    return PMPI_Win_allocate_shared(size, disp_unit, info, geopm_swap_comm_world(comm), baseptr, win);
}

int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    return PMPI_Win_create(base, size, disp_unit, info, geopm_swap_comm_world(comm), win);
}

int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win)
{
    return PMPI_Win_create_dynamic(info, geopm_swap_comm_world(comm), win);
}

