/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2014 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015-2017, Intel Corporation
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <string.h>

#include "config.h"
#include "geopm_fortran_strings.h"

#ifndef GEOPM_OMPI_BOTTOM
/// @todo fortran wrappers not working for OpenMPI
#ifdef GEOPM_OMPI_BOTTOM
    extern int MPI_FORTRAN_BOTTOM;
    extern int MPI_FORTRAN_IN_PLACE;
    extern int MPI_FORTRAN_UNWEIGHTED;
    extern int MPI_FORTRAN_WEIGHTS_EMPTY;
    extern int *MPI_F_STATUS_IGNORE;
    extern int *MPI_F_STATUSES_IGNORE;
    const int *MPIR_F_MPI_BOTTOM = &MPI_FORTRAN_BOTTOM;
    const int *MPIR_F_MPI_IN_PLACE = &MPI_FORTRAN_IN_PLACE;
    const int *MPIR_F_MPI_UNWEIGHTED = &MPI_FORTRAN_UNWEIGHTED;
    const int *MPIR_F_MPI_WEIGHTS_EMPTY = &MPI_FORTRAN_WEIGHTS_EMPTY;
#else
    extern void *MPIR_F_MPI_IN_PLACE;
    extern void *MPIR_F_MPI_BOTTOM;
    extern void *MPIR_F_MPI_UNWEIGHTED;
    extern void *MPIR_F_MPI_WEIGHTS_EMPTY;
    extern int *MPI_F_STATUS_IGNORE;
    extern int *MPI_F_STATUSES_IGNORE;
#endif

extern int c_int_to_f_logical(int *c_bool);

/* MPI_ALLGATHER Fortran wrappers */
static int geopm_mpi_allgather_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

void MPI_ALLGATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_allgather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_allgather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

/* MPI_ALLGATHERV Fortran wrappers */
static int geopm_mpi_allgatherv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm);
}

void MPI_ALLGATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

void mpi_allgatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
}

void mpi_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

void mpi_allgatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

/* MPI_ALLREDUCE Fortran wrappers */
static int geopm_mpi_allreduce_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    return MPI_Allreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

void MPI_ALLREDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allreduce_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_allreduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allreduce_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_allreduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allreduce_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_allreduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_allreduce_f(sendbuf, recvbuf, count, datatype, op, comm);
}

/* MPI_ALLTOALL Fortran wrappers */
static int geopm_mpi_alltoall_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

void MPI_ALLTOALL(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_alltoall(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_alltoall__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

/* MPI_ALLTOALLV Fortran wrappers */
static int geopm_mpi_alltoallv_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm);
}

void MPI_ALLTOALLV(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

void mpi_alltoallv(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

void mpi_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

void mpi_alltoallv__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

/* MPI_ALLTOALLW Fortran wrappers */
static int geopm_mpi_alltoallw_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm)
{
    int size;
    int err = 0;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    PMPI_Comm_size(c_comm, &size);
    MPI_Datatype *c_sendtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    MPI_Datatype *c_recvtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    if (c_sendtypes == NULL) {
        err =  MPI_ERR_OTHER;
    }
    if (!err && c_recvtypes == NULL) {
        free(c_sendtypes);
        err =  MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < size; ++i) {
            c_sendtypes[i] = PMPI_Type_f2c(sendtypes[i]);
            c_recvtypes[i] = PMPI_Type_f2c(recvtypes[i]);
        }
        err = MPI_Alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm);
        free(c_sendtypes);
        free(c_recvtypes);
    }
    return err;
}

void MPI_ALLTOALLW(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

void mpi_alltoallw(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

void mpi_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

void mpi_alltoallw__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

/* MPI_BARRIER Fortran wrappers */
static int geopm_mpi_barrier_f(MPI_Fint *comm)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Barrier(c_comm);
}

void MPI_BARRIER(MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_barrier_f(comm);
}

void mpi_barrier(MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_barrier_f(comm);
}

void mpi_barrier_(MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_barrier_f(comm);
}

void mpi_barrier__(MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_barrier_f(comm);
}

/* MPI_BCAST Fortran wrappers */
static int geopm_mpi_bcast_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Bcast(buf, *count, c_datatype, *root, c_comm);
}

void MPI_BCAST(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bcast_f(buf, count, datatype, root, comm);
}

void mpi_bcast(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bcast_f(buf, count, datatype, root, comm);
}

void mpi_bcast_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bcast_f(buf, count, datatype, root, comm);
}

void mpi_bcast__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bcast_f(buf, count, datatype, root, comm);
}

/* MPI_BSEND Fortran wrappers */
static int geopm_mpi_bsend_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Bsend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

void MPI_BSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_f(buf, count, datatype, dest, tag, comm);
}

void mpi_bsend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_f(buf, count, datatype, dest, tag, comm);
}

void mpi_bsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_f(buf, count, datatype, dest, tag, comm);
}

void mpi_bsend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_f(buf, count, datatype, dest, tag, comm);
}

/* MPI_BSEND_INIT Fortran wrappers */
static int geopm_mpi_bsend_init_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Bsend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_BSEND_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_bsend_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_bsend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_bsend_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_bsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_CART_COORDS Fortran wrappers */
static int geopm_mpi_cart_coords_f(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxdims, MPI_Fint *coords)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Cart_coords(c_comm, *rank, *maxdims, coords);
}

void MPI_CART_COORDS(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxdims, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_coords_f(comm, rank, maxdims, coords);
}

void mpi_cart_coords(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxdims, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_coords_f(comm, rank, maxdims, coords);
}

void mpi_cart_coords_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxdims, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_coords_f(comm, rank, maxdims, coords);
}

void mpi_cart_coords__(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxdims, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_coords_f(comm, rank, maxdims, coords);
}

/* MPI_CART_CREATE Fortran wrappers */
static int geopm_mpi_cart_create_f(MPI_Fint *old_comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart)
{
    MPI_Comm c_comm_cart;
    MPI_Comm c_old_comm = PMPI_Comm_f2c(*old_comm);
    int err = MPI_Cart_create(c_old_comm, *ndims, dims, periods, *reorder, &c_comm_cart);
    if (MPI_SUCCESS == err) {
        *comm_cart = PMPI_Comm_c2f(c_comm_cart);
    }
    return err;
}

void MPI_CART_CREATE(MPI_Fint *old_comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_create_f(old_comm, ndims, dims, periods, reorder, comm_cart);
}

void mpi_cart_create(MPI_Fint *old_comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_create_f(old_comm, ndims, dims, periods, reorder, comm_cart);
}

void mpi_cart_create_(MPI_Fint *old_comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_create_f(old_comm, ndims, dims, periods, reorder, comm_cart);
}

void mpi_cart_create__(MPI_Fint *old_comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *reorder, MPI_Fint *comm_cart, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_create_f(old_comm, ndims, dims, periods, reorder, comm_cart);
}

/* MPI_CARTDIM_GET Fortran wrappers */
static int geopm_mpi_cartdim_get_f(MPI_Fint *comm, MPI_Fint *ndims)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Cartdim_get(c_comm, ndims);
}

void MPI_CARTDIM_GET(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cartdim_get_f(comm, ndims);
}

void mpi_cartdim_get(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cartdim_get_f(comm, ndims);
}

void mpi_cartdim_get_(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cartdim_get_f(comm, ndims);
}

void mpi_cartdim_get__(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cartdim_get_f(comm, ndims);
}

/* MPI_CART_GET Fortran wrappers */
static int geopm_mpi_cart_get_f(MPI_Fint *comm, MPI_Fint *maxdims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *coords)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Cart_get(c_comm, *maxdims, dims, periods, coords);
}

void MPI_CART_GET(MPI_Fint *comm, MPI_Fint *maxdims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_get_f(comm, maxdims, dims, periods, coords);
}

void mpi_cart_get(MPI_Fint *comm, MPI_Fint *maxdims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_get_f(comm, maxdims, dims, periods, coords);
}

void mpi_cart_get_(MPI_Fint *comm, MPI_Fint *maxdims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_get_f(comm, maxdims, dims, periods, coords);
}

void mpi_cart_get__(MPI_Fint *comm, MPI_Fint *maxdims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *coords, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_get_f(comm, maxdims, dims, periods, coords);
}

/* MPI_CART_MAP Fortran wrappers */
static int geopm_mpi_cart_map_f(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *newrank)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Cart_map(c_comm, *ndims, dims, periods, newrank);
}

void MPI_CART_MAP(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_map_f(comm, ndims, dims, periods, newrank);
}

void mpi_cart_map(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_map_f(comm, ndims, dims, periods, newrank);
}

void mpi_cart_map_(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_map_f(comm, ndims, dims, periods, newrank);
}

void mpi_cart_map__(MPI_Fint *comm, MPI_Fint *ndims, MPI_Fint *dims, MPI_Fint *periods, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_map_f(comm, ndims, dims, periods, newrank);
}

/* MPI_CART_RANK Fortran wrappers */
static int geopm_mpi_cart_rank_f(MPI_Fint *comm, MPI_Fint *coords, MPI_Fint *rank)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Cart_rank(c_comm, coords, rank);
}

void MPI_CART_RANK(MPI_Fint *comm, MPI_Fint *coords, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_rank_f(comm, coords, rank);
}

void mpi_cart_rank(MPI_Fint *comm, MPI_Fint *coords, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_rank_f(comm, coords, rank);
}

void mpi_cart_rank_(MPI_Fint *comm, MPI_Fint *coords, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_rank_f(comm, coords, rank);
}

void mpi_cart_rank__(MPI_Fint *comm, MPI_Fint *coords, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_rank_f(comm, coords, rank);
}

/* MPI_CART_SHIFT Fortran wrappers */
static int geopm_mpi_cart_shift_f(MPI_Fint *comm, MPI_Fint *direction, MPI_Fint *disp, MPI_Fint *rank_source, MPI_Fint *rank_dest)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Cart_shift(c_comm, *direction, *disp, rank_source, rank_dest);
}

void MPI_CART_SHIFT(MPI_Fint *comm, MPI_Fint *direction, MPI_Fint *disp, MPI_Fint *rank_source, MPI_Fint *rank_dest, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_shift_f(comm, direction, disp, rank_source, rank_dest);
}

void mpi_cart_shift(MPI_Fint *comm, MPI_Fint *direction, MPI_Fint *disp, MPI_Fint *rank_source, MPI_Fint *rank_dest, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_shift_f(comm, direction, disp, rank_source, rank_dest);
}

void mpi_cart_shift_(MPI_Fint *comm, MPI_Fint *direction, MPI_Fint *disp, MPI_Fint *rank_source, MPI_Fint *rank_dest, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_shift_f(comm, direction, disp, rank_source, rank_dest);
}

void mpi_cart_shift__(MPI_Fint *comm, MPI_Fint *direction, MPI_Fint *disp, MPI_Fint *rank_source, MPI_Fint *rank_dest, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_shift_f(comm, direction, disp, rank_source, rank_dest);
}

/* MPI_CART_SUB Fortran wrappers */
static int geopm_mpi_cart_sub_f(MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *new_comm)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_new_comm;
    int err = MPI_Cart_sub(c_comm, remain_dims, &c_new_comm);
    if (MPI_SUCCESS == err) {
        *new_comm = PMPI_Comm_c2f(c_new_comm);
    }
    return err;
}

void MPI_CART_SUB(MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *new_comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_sub_f(comm, remain_dims, new_comm);
}

void mpi_cart_sub(MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *new_comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_sub_f(comm, remain_dims, new_comm);
}

void mpi_cart_sub_(MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *new_comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_sub_f(comm, remain_dims, new_comm);
}

void mpi_cart_sub__(MPI_Fint *comm, MPI_Fint *remain_dims, MPI_Fint *new_comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_cart_sub_f(comm, remain_dims, new_comm);
}

/* MPI_COMM_ACCEPT Fortran wrappers */
static int geopm_mpi_comm_accept_f(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, int port_name_len)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_newcomm;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    char *c_port_name;
    geopm_fortran_string_f2c(port_name, port_name_len, &c_port_name);
    int err = MPI_Comm_accept(c_port_name, c_info, *root, c_comm, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    free(c_port_name);
    return err;
}

void MPI_COMM_ACCEPT(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_accept_f(port_name, info, root, comm, newcomm, port_name_len);
}

void mpi_comm_accept(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_accept_f(port_name, info, root, comm, newcomm, port_name_len);
}

void mpi_comm_accept_(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_accept_f(port_name, info, root, comm, newcomm, port_name_len);
}

void mpi_comm_accept__(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_accept_f(port_name, info, root, comm, newcomm, port_name_len);
}

/* MPI_COMM_CALL_ERRHANDLER Fortran wrappers */
static int geopm_mpi_comm_call_errhandler_f(MPI_Fint *comm, MPI_Fint *errorcode)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_call_errhandler(c_comm, *errorcode);
}

void MPI_COMM_CALL_ERRHANDLER(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_call_errhandler_f(comm, errorcode);
}

void mpi_comm_call_errhandler(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_call_errhandler_f(comm, errorcode);
}

void mpi_comm_call_errhandler_(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_call_errhandler_f(comm, errorcode);
}

void mpi_comm_call_errhandler__(MPI_Fint *comm, MPI_Fint *errorcode, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_call_errhandler_f(comm, errorcode);
}

/* MPI_COMM_COMPARE Fortran wrappers */
static int geopm_mpi_comm_compare_f(MPI_Fint *comm1, MPI_Fint *comm2, MPI_Fint *result)
{
    MPI_Comm c_comm1 = PMPI_Comm_f2c(*comm1);
    MPI_Comm c_comm2 = PMPI_Comm_f2c(*comm2);
    return MPI_Comm_compare(c_comm1, c_comm2, result);
}

void MPI_COMM_COMPARE(MPI_Fint *comm1, MPI_Fint *comm2, MPI_Fint *result, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_compare_f(comm1, comm2, result);
}

void mpi_comm_compare(MPI_Fint *comm1, MPI_Fint *comm2, MPI_Fint *result, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_compare_f(comm1, comm2, result);
}

void mpi_comm_compare_(MPI_Fint *comm1, MPI_Fint *comm2, MPI_Fint *result, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_compare_f(comm1, comm2, result);
}

void mpi_comm_compare__(MPI_Fint *comm1, MPI_Fint *comm2, MPI_Fint *result, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_compare_f(comm1, comm2, result);
}

/* MPI_COMM_CONNECT Fortran wrappers */
static int geopm_mpi_comm_connect_f(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, int port_name_len)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_newcomm;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    char *c_port_name;
    geopm_fortran_string_f2c(port_name, port_name_len, &c_port_name);
    int err = MPI_Comm_connect(c_port_name, c_info, *root, c_comm, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = MPI_Comm_c2f(c_newcomm);
    }
    free(c_port_name);
    return err;
}

void MPI_COMM_CONNECT(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_connect_f(port_name, info, root, comm, newcomm, port_name_len);
}

void mpi_comm_connect(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_connect_f(port_name, info, root, comm, newcomm, port_name_len);
}

void mpi_comm_connect_(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_connect_f(port_name, info, root, comm, newcomm, port_name_len);
}

void mpi_comm_connect__(char *port_name, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr, int port_name_len)
{
    *ierr =  geopm_mpi_comm_connect_f(port_name, info, root, comm, newcomm, port_name_len);
}

/* MPI_COMM_CREATE_GROUP Fortran wrappers */
static int geopm_mpi_comm_create_group_f(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *tag, MPI_Fint *newcomm)
{
    MPI_Group c_group = PMPI_Group_f2c(*group);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_create_group(c_comm, c_group, *tag, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_COMM_CREATE_GROUP(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *tag, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_group_f(comm, group, tag, newcomm);
}

void mpi_comm_create_group(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *tag, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_group_f(comm, group, tag, newcomm);
}

void mpi_comm_create_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *tag, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_group_f(comm, group, tag, newcomm);
}

void mpi_comm_create_group__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *tag, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_group_f(comm, group, tag, newcomm);
}

/* MPI_COMM_CREATE Fortran wrappers */
static int geopm_mpi_comm_create_f(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm)
{
    MPI_Group c_group = PMPI_Group_f2c(*group);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_create(c_comm, c_group, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_COMM_CREATE(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_f(comm, group, newcomm);
}

void mpi_comm_create(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_f(comm, group, newcomm);
}

void mpi_comm_create_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_f(comm, group, newcomm);
}

void mpi_comm_create__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_create_f(comm, group, newcomm);
}

/* MPI_COMM_DELETE_ATTR Fortran wrappers */
static int geopm_mpi_comm_delete_attr_f(MPI_Fint *comm, MPI_Fint *comm_keyval)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_delete_attr(c_comm, *comm_keyval);
}

void MPI_COMM_DELETE_ATTR(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_delete_attr_f(comm, comm_keyval);
}

void mpi_comm_delete_attr(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_delete_attr_f(comm, comm_keyval);
}

void mpi_comm_delete_attr_(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_delete_attr_f(comm, comm_keyval);
}

void mpi_comm_delete_attr__(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_delete_attr_f(comm, comm_keyval);
}

/* MPI_COMM_DUP Fortran wrappers */
static int geopm_mpi_comm_dup_f(MPI_Fint *comm, MPI_Fint *newcomm)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_dup(c_comm, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_COMM_DUP(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_f(comm, newcomm);
}

void mpi_comm_dup(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_f(comm, newcomm);
}

void mpi_comm_dup_(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_f(comm, newcomm);
}

void mpi_comm_dup__(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_f(comm, newcomm);
}

/* MPI_COMM_DUP_WITH_INFO Fortran wrappers */
static int geopm_mpi_comm_dup_with_info_f(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_dup_with_info(c_comm, c_info, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_COMM_DUP_WITH_INFO(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_with_info_f(comm, info, newcomm);
}

void mpi_comm_dup_with_info(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_with_info_f(comm, info, newcomm);
}

void mpi_comm_dup_with_info_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_with_info_f(comm, info, newcomm);
}

void mpi_comm_dup_with_info__(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_dup_with_info_f(comm, info, newcomm);
}

/* MPI_COMM_GET_ATTR Fortran wrappers */
static int geopm_mpi_comm_get_attr_f(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *flag)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err =  MPI_Comm_get_attr(c_comm, *comm_keyval, attribute_val, flag);
    *flag = c_int_to_f_logical(flag);
    return err;
}

void MPI_COMM_GET_ATTR(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_attr_f(comm, comm_keyval, attribute_val, flag);
}

void mpi_comm_get_attr(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_attr_f(comm, comm_keyval, attribute_val, flag);
}

void mpi_comm_get_attr_(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_attr_f(comm, comm_keyval, attribute_val, flag);
}

void mpi_comm_get_attr__(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_attr_f(comm, comm_keyval, attribute_val, flag);
}

/* MPI_COMM_GET_ERRHANDLER Fortran wrappers */
static int geopm_mpi_comm_get_errhandler_f(MPI_Fint *comm, MPI_Fint *erhandler)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Errhandler c_erhandler;
    int err = MPI_Comm_get_errhandler(c_comm, &c_erhandler);
    *erhandler = MPI_Errhandler_c2f(c_erhandler);
    return err;
}

void MPI_COMM_GET_ERRHANDLER(MPI_Fint *comm, MPI_Fint *erhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_errhandler_f(comm, erhandler);
}

void mpi_comm_get_errhandler(MPI_Fint *comm, MPI_Fint *erhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_errhandler_f(comm, erhandler);
}

void mpi_comm_get_errhandler_(MPI_Fint *comm, MPI_Fint *erhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_errhandler_f(comm, erhandler);
}

void mpi_comm_get_errhandler__(MPI_Fint *comm, MPI_Fint *erhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_errhandler_f(comm, erhandler);
}

/* MPI_COMM_GET_INFO Fortran wrappers */
static int geopm_mpi_comm_get_info_f(MPI_Fint *comm, MPI_Fint *info_used)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Info c_info_used;
    int err = MPI_Comm_get_info(c_comm, &c_info_used);
    if (MPI_SUCCESS == err) {
        *info_used = PMPI_Info_c2f(c_info_used);
    }
    return err;
}

void MPI_COMM_GET_INFO(MPI_Fint *comm, MPI_Fint *info_used, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_info_f(comm, info_used);
}

void mpi_comm_get_info(MPI_Fint *comm, MPI_Fint *info_used, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_info_f(comm, info_used);
}

void mpi_comm_get_info_(MPI_Fint *comm, MPI_Fint *info_used, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_info_f(comm, info_used);
}

void mpi_comm_get_info__(MPI_Fint *comm, MPI_Fint *info_used, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_info_f(comm, info_used);
}

/* MPI_COMM_GET_NAME Fortran wrappers */
static int geopm_mpi_comm_get_name_f(MPI_Fint *comm, char *comm_name, MPI_Fint *resultlen, int name_len)
{
    char c_comm_name[MPI_MAX_OBJECT_NAME];
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err =  MPI_Comm_get_name(c_comm, c_comm_name, resultlen);
    if (MPI_SUCCESS == err) {
        geopm_fortran_string_c2f(c_comm_name, comm_name, name_len);
    }
    return err;
}

void MPI_COMM_GET_NAME(MPI_Fint *comm,  char *comm_name, MPI_Fint *resultlen, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_get_name_f(comm, comm_name, resultlen, name_len);
}

void mpi_comm_get_name(MPI_Fint *comm,  char *comm_name, MPI_Fint *resultlen, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_get_name_f(comm, comm_name, resultlen, name_len);
}

void mpi_comm_get_name_(MPI_Fint *comm,  char *comm_name, MPI_Fint *resultlen, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_get_name_f(comm, comm_name, resultlen, name_len);
}

void mpi_comm_get_name__(MPI_Fint *comm,  char *comm_name, MPI_Fint *resultlen, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_get_name_f(comm, comm_name, resultlen, name_len);
}

/* MPI_COMM_GET_PARENT Fortran wrappers */
static int geopm_mpi_comm_get_parent_f(MPI_Fint *parent)
{
    MPI_Comm c_parent;
    int err = MPI_Comm_get_parent(&c_parent);
    if (MPI_SUCCESS == err) {
        *parent = PMPI_Comm_c2f(c_parent);
    }
    return err;
}

void MPI_COMM_GET_PARENT(MPI_Fint *parent, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_parent_f(parent);
}

void mpi_comm_get_parent(MPI_Fint *parent, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_parent_f(parent);
}

void mpi_comm_get_parent_(MPI_Fint *parent, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_parent_f(parent);
}

void mpi_comm_get_parent__(MPI_Fint *parent, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_get_parent_f(parent);
}

/* MPI_COMM_GROUP Fortran wrappers */
static int geopm_mpi_comm_group_f(MPI_Fint *comm, MPI_Fint *group)
{
    MPI_Group c_group;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Comm_group(c_comm, &c_group);
    if (MPI_SUCCESS == err) {
        *group = PMPI_Group_c2f(c_group);
    }
    return err;
}

void MPI_COMM_GROUP(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_group_f(comm, group);
}

void mpi_comm_group(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_group_f(comm, group);
}

void mpi_comm_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_group_f(comm, group);
}

void mpi_comm_group__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_group_f(comm, group);
}

/* MPI_COMM_IDUP Fortran wrappers */
static int geopm_mpi_comm_idup_f(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request)
{
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_idup(c_comm, &c_newcomm, &c_request);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_COMM_IDUP(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_idup_f(comm, newcomm, request);
}

void mpi_comm_idup(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_idup_f(comm, newcomm, request);
}

void mpi_comm_idup_(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_idup_f(comm, newcomm, request);
}

void mpi_comm_idup__(MPI_Fint *comm, MPI_Fint *newcomm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_idup_f(comm, newcomm, request);
}

/* MPI_COMM_RANK Fortran wrappers */
static int geopm_mpi_comm_rank_f(MPI_Fint *comm, MPI_Fint *rank)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_rank(c_comm, rank);
}

void MPI_COMM_RANK(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_rank_f(comm, rank);
}

void mpi_comm_rank(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_rank_f(comm, rank);
}

void mpi_comm_rank_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_rank_f(comm, rank);
}

void mpi_comm_rank__(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_rank_f(comm, rank);
}

/* MPI_COMM_REMOTE_GROUP Fortran wrappers */
static int geopm_mpi_comm_remote_group_f(MPI_Fint *comm, MPI_Fint *group)
{
    MPI_Group c_group;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Comm_remote_group(c_comm, &c_group);
    if (MPI_SUCCESS == err) {
        *group = PMPI_Group_c2f(c_group);
    }
    return err;
}

void MPI_COMM_REMOTE_GROUP(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_group_f(comm, group);
}

void mpi_comm_remote_group(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_group_f(comm, group);
}

void mpi_comm_remote_group_(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_group_f(comm, group);
}

void mpi_comm_remote_group__(MPI_Fint *comm, MPI_Fint *group, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_group_f(comm, group);
}

/* MPI_COMM_REMOTE_SIZE Fortran wrappers */
static int geopm_mpi_comm_remote_size_f(MPI_Fint *comm, MPI_Fint *size)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_remote_size(c_comm, size);
}

void MPI_COMM_REMOTE_SIZE(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_size_f(comm, size);
}

void mpi_comm_remote_size(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_size_f(comm, size);
}

void mpi_comm_remote_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_size_f(comm, size);
}

void mpi_comm_remote_size__(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_remote_size_f(comm, size);
}

/* MPI_COMM_SET_ATTR Fortran wrappers */
static int geopm_mpi_comm_set_attr_f(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_set_attr(c_comm, *comm_keyval, attribute_val);
}

void MPI_COMM_SET_ATTR(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_attr_f(comm, comm_keyval, attribute_val);
}

void mpi_comm_set_attr(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_attr_f(comm, comm_keyval, attribute_val);
}

void mpi_comm_set_attr_(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_attr_f(comm, comm_keyval, attribute_val);
}

void mpi_comm_set_attr__(MPI_Fint *comm, MPI_Fint *comm_keyval, MPI_Fint *attribute_val, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_attr_f(comm, comm_keyval, attribute_val);
}

/* MPI_COMM_SET_ERRHANDLER Fortran wrappers */
static int geopm_mpi_comm_set_errhandler_f(MPI_Fint *comm, MPI_Fint *errhandler)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Errhandler c_errhandler = MPI_Errhandler_f2c(*errhandler);
    return MPI_Comm_set_errhandler(c_comm, c_errhandler);
}

void MPI_COMM_SET_ERRHANDLER(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_errhandler_f(comm, errhandler);
}

void mpi_comm_set_errhandler(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_errhandler_f(comm, errhandler);
}

void mpi_comm_set_errhandler_(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_errhandler_f(comm, errhandler);
}

void mpi_comm_set_errhandler__(MPI_Fint *comm, MPI_Fint *errhandler, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_errhandler_f(comm, errhandler);
}

/* MPI_COMM_SET_INFO Fortran wrappers */
static int geopm_mpi_comm_set_info_f(MPI_Fint *comm, MPI_Fint *info)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_set_info(c_comm, c_info);
}

void MPI_COMM_SET_INFO(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_info_f(comm, info);
}

void mpi_comm_set_info(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_info_f(comm, info);
}

void mpi_comm_set_info_(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_info_f(comm, info);
}

void mpi_comm_set_info__(MPI_Fint *comm, MPI_Fint *info, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_set_info_f(comm, info);
}

/* MPI_COMM_SET_NAME Fortran wrappers */
static int geopm_mpi_comm_set_name_f(MPI_Fint *comm,  char *comm_name, int name_len)
{
    char *c_comm_name;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    geopm_fortran_string_f2c(comm_name, name_len, &c_comm_name);
    int err =  MPI_Comm_set_name(c_comm, c_comm_name);
    free(c_comm_name);
    return err;
}

void MPI_COMM_SET_NAME(MPI_Fint *comm,  char *comm_name, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_set_name_f(comm, comm_name, name_len);
}

void mpi_comm_set_name(MPI_Fint *comm,  char *comm_name, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_set_name_f(comm, comm_name, name_len);
}

void mpi_comm_set_name_(MPI_Fint *comm,  char *comm_name, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_set_name_f(comm, comm_name, name_len);
}

void mpi_comm_set_name__(MPI_Fint *comm,  char *comm_name, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_comm_set_name_f(comm, comm_name, name_len);
}

/* MPI_COMM_SIZE Fortran wrappers */
static int geopm_mpi_comm_size_f(MPI_Fint *comm, MPI_Fint *size)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Comm_size(c_comm, size);
}

void MPI_COMM_SIZE(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_size_f(comm, size);
}

void mpi_comm_size(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_size_f(comm, size);
}

void mpi_comm_size_(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_size_f(comm, size);
}

void mpi_comm_size__(MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_size_f(comm, size);
}

/* MPI_COMM_SPAWN Fortran wrappers */
static int geopm_mpi_comm_spawn_f(char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, int cmd_len, int string_len)
{
    char **c_argv = MPI_ARGV_NULL;
    char *c_command = NULL;
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_intercomm;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    geopm_fortran_string_f2c(command, cmd_len, &c_command);
    if (argv != NULL) {
        geopm_fortran_argv_f2c(argv, string_len, string_len, &c_argv);
    }
    int err = MPI_Comm_spawn(c_command, c_argv, *maxprocs, c_info, *root, c_comm, &c_intercomm, array_of_errcodes);
    if (MPI_SUCCESS == err) {
        *intercomm = PMPI_Comm_c2f(c_intercomm);
    }
    free(c_command);
    geopm_argv_free(c_argv);
    return err;
}

void MPI_COMM_SPAWN(char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_len, int string_len)
{
    *ierr =  geopm_mpi_comm_spawn_f(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, cmd_len, string_len);
}

void mpi_comm_spawn(char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_len, int string_len)
{
    *ierr =  geopm_mpi_comm_spawn_f(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, cmd_len, string_len);
}

void mpi_comm_spawn_(char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_len, int string_len)
{
    *ierr =  geopm_mpi_comm_spawn_f(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, cmd_len, string_len);
}

void mpi_comm_spawn__(char *command, char *argv, MPI_Fint *maxprocs, MPI_Fint *info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_len, int string_len)
{
    *ierr =  geopm_mpi_comm_spawn_f(command, argv, maxprocs, info, root, comm, intercomm, array_of_errcodes, cmd_len, string_len);
}

/* MPI_COMM_SPAWN_MULTIPLE Fortran wrappers */
static int geopm_mpi_comm_spawn_multiple_f(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI_Fint *array_of_maxprocs, MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, int cmd_string_len, int argv_string_len)
{
    int err = 0;
    char **c_array_of_commands = NULL;
    char ***c_array_of_argv = NULL;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_intercomm;
    MPI_Info *c_array_of_info = (MPI_Info *)malloc(*count * sizeof(MPI_Info));
    if (c_array_of_info == NULL) {
        err = MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < *count; ++i) {
            c_array_of_info[i] = PMPI_Info_f2c(array_of_info[i]);
        }
        if (array_of_argv != NULL) {
            geopm_fortran_multiple_argvs_f2c(*count, array_of_argv, argv_string_len, &c_array_of_argv);
        }
        geopm_fortran_argv_f2c(array_of_commands, cmd_string_len, cmd_string_len, &c_array_of_commands);
        err = MPI_Comm_spawn_multiple(*count, c_array_of_commands, c_array_of_argv, array_of_maxprocs, c_array_of_info, *root, c_comm, &c_intercomm, array_of_errcodes);
        if (MPI_SUCCESS == err) {
            *intercomm = PMPI_Comm_c2f(c_intercomm);
        }
        free(c_array_of_info);
        geopm_argv_free(c_array_of_commands);
        for (int i = 0; i < *count; ++i) {
            geopm_argv_free(c_array_of_argv[i]);
        }
        free(c_array_of_argv);
    }
    return err;
}

void MPI_COMM_SPAWN_MULTIPLE(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI_Fint *array_of_maxprocs, MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_string_len, int argv_string_len)
{
    *ierr =  geopm_mpi_comm_spawn_multiple_f(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, cmd_string_len, argv_string_len);
}

void mpi_comm_spawn_multiple(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI_Fint *array_of_maxprocs, MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_string_len, int argv_string_len)
{
    *ierr =  geopm_mpi_comm_spawn_multiple_f(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, cmd_string_len, argv_string_len);
}

void mpi_comm_spawn_multiple_(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI_Fint *array_of_maxprocs, MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_string_len, int argv_string_len)
{
    *ierr =  geopm_mpi_comm_spawn_multiple_f(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, cmd_string_len, argv_string_len);
}

void mpi_comm_spawn_multiple__(MPI_Fint *count, char *array_of_commands, char *array_of_argv, MPI_Fint *array_of_maxprocs, MPI_Fint *array_of_info, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *intercomm, MPI_Fint *array_of_errcodes, MPI_Fint *ierr, int cmd_string_len, int argv_string_len)
{
    *ierr =  geopm_mpi_comm_spawn_multiple_f(count, array_of_commands, array_of_argv, array_of_maxprocs, array_of_info, root, comm, intercomm, array_of_errcodes, cmd_string_len, argv_string_len);
}

/* MPI_COMM_SPLIT Fortran wrappers */
static int geopm_mpi_comm_split_f(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_split(c_comm, *color, *key, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_COMM_SPLIT(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_f(comm, color, key, newcomm);
}

void mpi_comm_split(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_f(comm, color, key, newcomm);
}

void mpi_comm_split_(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_f(comm, color, key, newcomm);
}

void mpi_comm_split__(MPI_Fint *comm, MPI_Fint *color, MPI_Fint *key, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_f(comm, color, key, newcomm);
}

/* MPI_COMM_SPLIT_TYPE Fortran wrappers */
static int geopm_mpi_comm_split_type_f(MPI_Fint *comm, MPI_Fint *split_type, MPI_Fint *key, MPI_Fint *info, MPI_Fint *newcomm)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Comm c_newcomm;
    int err = MPI_Comm_split_type(c_comm, *split_type, *key, c_info, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_COMM_SPLIT_TYPE(MPI_Fint *comm, MPI_Fint *split_type, MPI_Fint *key, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_type_f(comm, split_type, key, info, newcomm);
}

void mpi_comm_split_type(MPI_Fint *comm, MPI_Fint *split_type, MPI_Fint *key, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_type_f(comm, split_type, key, info, newcomm);
}

void mpi_comm_split_type_(MPI_Fint *comm, MPI_Fint *split_type, MPI_Fint *key, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_type_f(comm, split_type, key, info, newcomm);
}

void mpi_comm_split_type__(MPI_Fint *comm, MPI_Fint *split_type, MPI_Fint *key, MPI_Fint *info, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_split_type_f(comm, split_type, key, info, newcomm);
}

/* MPI_COMM_TEST_INTER Fortran wrappers */
static int geopm_mpi_comm_test_inter_f(MPI_Fint *comm, MPI_Fint *flag)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err =  MPI_Comm_test_inter(c_comm, flag);
    *flag = c_int_to_f_logical(flag);
    return err;
}

void MPI_COMM_TEST_INTER(MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_test_inter_f(comm, flag);
}

void mpi_comm_test_inter(MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_test_inter_f(comm, flag);
}

void mpi_comm_test_inter_(MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_test_inter_f(comm, flag);
}

void mpi_comm_test_inter__(MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_comm_test_inter_f(comm, flag);
}

/* MPI_DIST_GRAPH_CREATE_ADJACENT Fortran wrappers */
static int geopm_mpi_dist_graph_create_adjacent_f(MPI_Fint *comm_old, MPI_Fint *indegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *outdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *comm_dist_graph)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
    MPI_Comm c_comm_dist_graph;
    if (sourceweights ==  MPIR_F_MPI_UNWEIGHTED) {
        sourceweights = MPI_UNWEIGHTED;
    }
    else if (sourceweights == MPIR_F_MPI_WEIGHTS_EMPTY) {
        sourceweights = MPI_WEIGHTS_EMPTY;
    }
    if (destweights ==  MPIR_F_MPI_UNWEIGHTED) {
        destweights = MPI_UNWEIGHTED;
    }
    else if (destweights == MPIR_F_MPI_WEIGHTS_EMPTY) {
        destweights = MPI_WEIGHTS_EMPTY;
    }
    int err = MPI_Dist_graph_create_adjacent(c_comm_old, *indegree, sources, sourceweights, *outdegree, destinations, destweights, c_info, *reorder, &c_comm_dist_graph);
    if (MPI_SUCCESS == err) {
        *comm_dist_graph = PMPI_Comm_c2f(c_comm_dist_graph);
    }
    return err;
}

void MPI_DIST_GRAPH_CREATE_ADJACENT(MPI_Fint *comm_old, MPI_Fint *indegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *outdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *comm_dist_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_adjacent_f(comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph);
}

void mpi_dist_graph_create_adjacent(MPI_Fint *comm_old, MPI_Fint *indegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *outdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *comm_dist_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_adjacent_f(comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph);
}

void mpi_dist_graph_create_adjacent_(MPI_Fint *comm_old, MPI_Fint *indegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *outdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *comm_dist_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_adjacent_f(comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph);
}

void mpi_dist_graph_create_adjacent__(MPI_Fint *comm_old, MPI_Fint *indegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *outdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *comm_dist_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_adjacent_f(comm_old, indegree, sources, sourceweights, outdegree, destinations, destweights, info, reorder, comm_dist_graph);
}

/* MPI_DIST_GRAPH_CREATE Fortran wrappers */
static int geopm_mpi_dist_graph_create_f(MPI_Fint *comm_old, MPI_Fint *n, MPI_Fint *nodes, MPI_Fint *degrees, MPI_Fint *targets, MPI_Fint *weights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *newcomm)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
    MPI_Comm c_newcomm;
    if (weights ==  MPIR_F_MPI_UNWEIGHTED) {
        weights = MPI_UNWEIGHTED;
    }
    else if (weights == MPIR_F_MPI_WEIGHTS_EMPTY) {
        weights = MPI_WEIGHTS_EMPTY;
    }
    int err = MPI_Dist_graph_create(c_comm_old, *n, nodes, degrees, targets, weights, c_info, *reorder, &c_newcomm);
    if (MPI_SUCCESS == err) {
        *newcomm = PMPI_Comm_c2f(c_newcomm);
    }
    return err;
}

void MPI_DIST_GRAPH_CREATE(MPI_Fint *comm_old, MPI_Fint *n, MPI_Fint *nodes, MPI_Fint *degrees, MPI_Fint *targets, MPI_Fint *weights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_f(comm_old, n, nodes, degrees, targets, weights, info, reorder, newcomm);
}

void mpi_dist_graph_create(MPI_Fint *comm_old, MPI_Fint *n, MPI_Fint *nodes, MPI_Fint *degrees, MPI_Fint *targets, MPI_Fint *weights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_f(comm_old, n, nodes, degrees, targets, weights, info, reorder, newcomm);
}

void mpi_dist_graph_create_(MPI_Fint *comm_old, MPI_Fint *n, MPI_Fint *nodes, MPI_Fint *degrees, MPI_Fint *targets, MPI_Fint *weights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_f(comm_old, n, nodes, degrees, targets, weights, info, reorder, newcomm);
}

void mpi_dist_graph_create__(MPI_Fint *comm_old, MPI_Fint *n, MPI_Fint *nodes, MPI_Fint *degrees, MPI_Fint *targets, MPI_Fint *weights, MPI_Fint *info, MPI_Fint *reorder, MPI_Fint *newcomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_create_f(comm_old, n, nodes, degrees, targets, weights, info, reorder, newcomm);
}

/* MPI_DIST_GRAPH_NEIGHBORS_COUNT Fortran wrappers */
static int geopm_mpi_dist_graph_neighbors_count_f(MPI_Fint *comm, MPI_Fint *inneighbors, MPI_Fint *outneighbors, MPI_Fint *weighted)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err =  MPI_Dist_graph_neighbors_count(c_comm, inneighbors, outneighbors, weighted);
    *weighted = c_int_to_f_logical(weighted);
    return err;
}

void MPI_DIST_GRAPH_NEIGHBORS_COUNT(MPI_Fint *comm, MPI_Fint *inneighbors, MPI_Fint *outneighbors, MPI_Fint *weighted, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_count_f(comm, inneighbors, outneighbors, weighted);
}

void mpi_dist_graph_neighbors_count(MPI_Fint *comm, MPI_Fint *inneighbors, MPI_Fint *outneighbors, MPI_Fint *weighted, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_count_f(comm, inneighbors, outneighbors, weighted);
}

void mpi_dist_graph_neighbors_count_(MPI_Fint *comm, MPI_Fint *inneighbors, MPI_Fint *outneighbors, MPI_Fint *weighted, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_count_f(comm, inneighbors, outneighbors, weighted);
}

void mpi_dist_graph_neighbors_count__(MPI_Fint *comm, MPI_Fint *inneighbors, MPI_Fint *outneighbors, MPI_Fint *weighted, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_count_f(comm, inneighbors, outneighbors, weighted);
}

/* MPI_DIST_GRAPH_NEIGHBORS Fortran wrappers */
static int geopm_mpi_dist_graph_neighbors_f(MPI_Fint *comm, MPI_Fint *maxindegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *maxoutdegree, MPI_Fint *destinations, MPI_Fint *destweights)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Dist_graph_neighbors(c_comm, *maxindegree, sources, sourceweights, *maxoutdegree, destinations, destweights);
}

void MPI_DIST_GRAPH_NEIGHBORS(MPI_Fint *comm, MPI_Fint *maxindegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *maxoutdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_f(comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights);
}

void mpi_dist_graph_neighbors(MPI_Fint *comm, MPI_Fint *maxindegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *maxoutdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_f(comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights);
}

void mpi_dist_graph_neighbors_(MPI_Fint *comm, MPI_Fint *maxindegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *maxoutdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_f(comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights);
}

void mpi_dist_graph_neighbors__(MPI_Fint *comm, MPI_Fint *maxindegree, MPI_Fint *sources, MPI_Fint *sourceweights, MPI_Fint *maxoutdegree, MPI_Fint *destinations, MPI_Fint *destweights, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_dist_graph_neighbors_f(comm, maxindegree, sources, sourceweights, maxoutdegree, destinations, destweights);
}

/* MPI_EXSCAN Fortran wrappers */
static int geopm_mpi_exscan_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    return MPI_Exscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

void MPI_EXSCAN(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_exscan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_exscan(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_exscan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_exscan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_exscan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_exscan__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_exscan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

/* MPI_FILE_OPEN Fortran wrappers */
static int geopm_mpi_file_open_f(MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_Fint *fh, int name_len)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_File c_fh;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    char *c_filename;
    geopm_fortran_string_f2c(filename, name_len, &c_filename);
    int err = MPI_File_open(c_comm, c_filename, *amode, c_info, &c_fh);
    if (MPI_SUCCESS == err) {
        *fh = PMPI_File_c2f(c_fh);
    }
    free(c_filename);
    return err;
}

void MPI_FILE_OPEN(MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_Fint *fh, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_file_open_f(comm, filename, amode, info, fh, name_len);
}

void mpi_file_open(MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_Fint *fh, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_file_open_f(comm, filename, amode, info, fh, name_len);
}

void mpi_file_open_(MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_Fint *fh, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_file_open_f(comm, filename, amode, info, fh, name_len);
}

void mpi_file_open__(MPI_Fint *comm, char *filename, MPI_Fint *amode, MPI_Fint *info, MPI_Fint *fh, MPI_Fint *ierr, int name_len)
{
    *ierr =  geopm_mpi_file_open_f(comm, filename, amode, info, fh, name_len);
}

/* MPI_FINALIZE Fortran wrappers */
static int geopm_mpi_finalize_f(void)
{
    return MPI_Finalize();
}

void MPI_FINALIZE(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_finalize_f();
}

void mpi_finalize(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_finalize_f();
}

void mpi_finalize_(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_finalize_f();
}

void mpi_finalize__(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_finalize_f();
}

/* MPI_GATHER Fortran wrappers */
static int geopm_mpi_gather_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Gather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

void MPI_GATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_gather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_gather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_gather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

/* MPI_GATHERV Fortran wrappers */
static int geopm_mpi_gatherv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Gatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm);
}

void MPI_GATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}

void mpi_gatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}

void mpi_gatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}

void mpi_gatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_gatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm);
}

/* MPI_GRAPH_CREATE Fortran wrappers */
static int geopm_mpi_graph_create_f(MPI_Fint *comm_old, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *reorder, MPI_Fint *comm_graph)
{
    MPI_Comm c_comm_old = PMPI_Comm_f2c(*comm_old);
    MPI_Comm c_comm_graph;
    int err = MPI_Graph_create(c_comm_old, *nnodes, index, edges, *reorder, &c_comm_graph);
    if (MPI_SUCCESS == err) {
        *comm_graph = PMPI_Comm_c2f(c_comm_graph);
    }
    return err;
}

void MPI_GRAPH_CREATE(MPI_Fint *comm_old, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *reorder, MPI_Fint *comm_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_create_f(comm_old, nnodes, index, edges, reorder, comm_graph);
}

void mpi_graph_create(MPI_Fint *comm_old, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *reorder, MPI_Fint *comm_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_create_f(comm_old, nnodes, index, edges, reorder, comm_graph);
}

void mpi_graph_create_(MPI_Fint *comm_old, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *reorder, MPI_Fint *comm_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_create_f(comm_old, nnodes, index, edges, reorder, comm_graph);
}

void mpi_graph_create__(MPI_Fint *comm_old, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *reorder, MPI_Fint *comm_graph, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_create_f(comm_old, nnodes, index, edges, reorder, comm_graph);
}

/* MPI_GRAPHDIMS_GET Fortran wrappers */
static int geopm_mpi_graphdims_get_f(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *nedges)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Graphdims_get(c_comm, nnodes, nedges);
}

void MPI_GRAPHDIMS_GET(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *nedges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graphdims_get_f(comm, nnodes, nedges);
}

void mpi_graphdims_get(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *nedges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graphdims_get_f(comm, nnodes, nedges);
}

void mpi_graphdims_get_(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *nedges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graphdims_get_f(comm, nnodes, nedges);
}

void mpi_graphdims_get__(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *nedges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graphdims_get_f(comm, nnodes, nedges);
}

/* MPI_GRAPH_GET Fortran wrappers */
static int geopm_mpi_graph_get_f(MPI_Fint *comm, MPI_Fint *maxindex, MPI_Fint *maxedges, MPI_Fint *index, MPI_Fint *edges)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Graph_get(c_comm, *maxindex, *maxedges, index, edges);
}

void MPI_GRAPH_GET(MPI_Fint *comm, MPI_Fint *maxindex, MPI_Fint *maxedges, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_get_f(comm, maxindex, maxedges, index, edges);
}

void mpi_graph_get(MPI_Fint *comm, MPI_Fint *maxindex, MPI_Fint *maxedges, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_get_f(comm, maxindex, maxedges, index, edges);
}

void mpi_graph_get_(MPI_Fint *comm, MPI_Fint *maxindex, MPI_Fint *maxedges, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_get_f(comm, maxindex, maxedges, index, edges);
}

void mpi_graph_get__(MPI_Fint *comm, MPI_Fint *maxindex, MPI_Fint *maxedges, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_get_f(comm, maxindex, maxedges, index, edges);
}

/* MPI_GRAPH_MAP Fortran wrappers */
static int geopm_mpi_graph_map_f(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *newrank)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Graph_map(c_comm, *nnodes, index, edges, newrank);
}

void MPI_GRAPH_MAP(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_map_f(comm, nnodes, index, edges, newrank);
}

void mpi_graph_map(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_map_f(comm, nnodes, index, edges, newrank);
}

void mpi_graph_map_(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_map_f(comm, nnodes, index, edges, newrank);
}

void mpi_graph_map__(MPI_Fint *comm, MPI_Fint *nnodes, MPI_Fint *index, MPI_Fint *edges, MPI_Fint *newrank, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_map_f(comm, nnodes, index, edges, newrank);
}

/* MPI_GRAPH_NEIGHBORS_COUNT Fortran wrappers */
static int geopm_mpi_graph_neighbors_count_f(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *nneighbors)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Graph_neighbors_count(c_comm, *rank, nneighbors);
}

void MPI_GRAPH_NEIGHBORS_COUNT(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *nneighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_count_f(comm, rank, nneighbors);
}

void mpi_graph_neighbors_count(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *nneighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_count_f(comm, rank, nneighbors);
}

void mpi_graph_neighbors_count_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *nneighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_count_f(comm, rank, nneighbors);
}

void mpi_graph_neighbors_count__(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *nneighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_count_f(comm, rank, nneighbors);
}

/* MPI_GRAPH_NEIGHBORS Fortran wrappers */
static int geopm_mpi_graph_neighbors_f(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxneighbors, MPI_Fint *neighbors)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Graph_neighbors(c_comm, *rank, *maxneighbors, neighbors);
}

void MPI_GRAPH_NEIGHBORS(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxneighbors, MPI_Fint *neighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_f(comm, rank, maxneighbors, neighbors);
}

void mpi_graph_neighbors(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxneighbors, MPI_Fint *neighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_f(comm, rank, maxneighbors, neighbors);
}

void mpi_graph_neighbors_(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxneighbors, MPI_Fint *neighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_f(comm, rank, maxneighbors, neighbors);
}

void mpi_graph_neighbors__(MPI_Fint *comm, MPI_Fint *rank, MPI_Fint *maxneighbors, MPI_Fint *neighbors, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_graph_neighbors_f(comm, rank, maxneighbors, neighbors);
}

/* MPI_IALLGATHER Fortran wrappers */
static int geopm_mpi_iallgather_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Iallgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IALLGATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_iallgather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_iallgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_iallgather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

/* MPI_IALLGATHERV Fortran wrappers */
static int geopm_mpi_iallgatherv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Iallgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = MPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IALLGATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

void mpi_iallgatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

void mpi_iallgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

void mpi_iallgatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

/* MPI_IALLREDUCE Fortran wrappers */
static int geopm_mpi_iallreduce_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    int err = MPI_Iallreduce(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IALLREDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallreduce_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iallreduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallreduce_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iallreduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallreduce_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iallreduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iallreduce_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

/* MPI_IALLTOALL Fortran wrappers */
static int geopm_mpi_ialltoall_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ialltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IALLTOALL(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ialltoall(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ialltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ialltoall__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

/* MPI_IALLTOALLV Fortran wrappers */
static int geopm_mpi_ialltoallv_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ialltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IALLTOALLV(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

void mpi_ialltoallv(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

void mpi_ialltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

void mpi_ialltoallv__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

/* MPI_IALLTOALLW Fortran wrappers */
static int geopm_mpi_ialltoallw_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request)
{
    int size;
    int err = 0;
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    PMPI_Comm_size(c_comm, &size);
    MPI_Datatype *c_sendtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    MPI_Datatype *c_recvtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    if (c_sendtypes == NULL) {
        err =  MPI_ERR_OTHER;
    }
    if (!err && c_recvtypes == NULL) {
        free(c_sendtypes);
        err =  MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < size; ++i) {
            c_sendtypes[i] = PMPI_Type_f2c(sendtypes[i]);
            c_recvtypes[i] = PMPI_Type_f2c(recvtypes[i]);
        }
        err = MPI_Ialltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, &c_request);
        if (MPI_SUCCESS == err) {
            *request = PMPI_Request_c2f(c_request);
        }
        free(c_sendtypes);
        free(c_recvtypes);
    }
    return err;
}

void MPI_IALLTOALLW(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

void mpi_ialltoallw(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

void mpi_ialltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

void mpi_ialltoallw__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ialltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

/* MPI_IBARRIER Fortran wrappers */
static int geopm_mpi_ibarrier_f(MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ibarrier(c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IBARRIER(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ibarrier_f(comm, request);
}

void mpi_ibarrier(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ibarrier_f(comm, request);
}

void mpi_ibarrier_(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ibarrier_f(comm, request);
}

void mpi_ibarrier__(MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ibarrier_f(comm, request);
}

/* MPI_IBCAST Fortran wrappers */
static int geopm_mpi_ibcast_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Request c_request;
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ibcast(buf, *count, c_datatype, *root, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IBCAST(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibcast_f(buf, count, datatype, root, comm, request);
}

void mpi_ibcast(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibcast_f(buf, count, datatype, root, comm, request);
}

void mpi_ibcast_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibcast_f(buf, count, datatype, root, comm, request);
}

void mpi_ibcast__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibcast_f(buf, count, datatype, root, comm, request);
}

/* MPI_IBSEND Fortran wrappers */
static int geopm_mpi_ibsend_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ibsend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IBSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibsend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_ibsend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibsend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_ibsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibsend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_ibsend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ibsend_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_IEXSCAN Fortran wrappers */
static int geopm_mpi_iexscan_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    int err = MPI_Iexscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IEXSCAN(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iexscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iexscan(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iexscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iexscan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iexscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iexscan__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iexscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

/* MPI_IGATHER Fortran wrappers */
static int geopm_mpi_igather_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Igather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IGATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_igather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_igather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_igather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

/* MPI_IGATHERV Fortran wrappers */
static int geopm_mpi_igatherv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Igatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, *root, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = MPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IGATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
}

void mpi_igatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
}

void mpi_igatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
}

void mpi_igatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_igatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, request);
}

/* MPI_IMPROBE Fortran wrappers */
static int geopm_mpi_improbe_f(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *message, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Message c_message;
    int err = MPI_Improbe(*source, *tag, c_comm, flag, &c_message, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE && *flag) {
        PMPI_Status_c2f(&c_status, status);
        *message = MPI_Message_c2f(c_message);
    }
    *flag = c_int_to_f_logical(flag);
    return err;
}

void MPI_IMPROBE(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_improbe_f(source, tag, comm, flag, message, status);
}

void mpi_improbe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_improbe_f(source, tag, comm, flag, message, status);
}

void mpi_improbe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_improbe_f(source, tag, comm, flag, message, status);
}

void mpi_improbe__(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_improbe_f(source, tag, comm, flag, message, status);
}

/* MPI_INEIGHBOR_ALLGATHER Fortran wrappers */
static int geopm_mpi_ineighbor_allgather_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ineighbor_allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_INEIGHBOR_ALLGATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ineighbor_allgather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ineighbor_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ineighbor_allgather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

/* MPI_INEIGHBOR_ALLGATHERV Fortran wrappers */
static int geopm_mpi_ineighbor_allgatherv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ineighbor_allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_INEIGHBOR_ALLGATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

void mpi_ineighbor_allgatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

void mpi_ineighbor_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

void mpi_ineighbor_allgatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, request);
}

/* MPI_INEIGHBOR_ALLTOALL Fortran wrappers */
static int geopm_mpi_ineighbor_alltoall_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ineighbor_alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_INEIGHBOR_ALLTOALL(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ineighbor_alltoall(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ineighbor_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

void mpi_ineighbor_alltoall__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request);
}

/* MPI_INEIGHBOR_ALLTOALLV Fortran wrappers */
static int geopm_mpi_ineighbor_alltoallv_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ineighbor_alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_INEIGHBOR_ALLTOALLV(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

void mpi_ineighbor_alltoallv(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

void mpi_ineighbor_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

void mpi_ineighbor_alltoallv__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm, request);
}

/* MPI_INEIGHBOR_ALLTOALLW Fortran wrappers */
static int geopm_mpi_ineighbor_alltoallw_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request)
{
    int size;
    int err = 0;
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    PMPI_Comm_size(c_comm, &size);
    MPI_Datatype *c_sendtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    MPI_Datatype *c_recvtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    if (c_sendtypes == NULL) {
        err =  MPI_ERR_OTHER;
    }
    if (!err && c_recvtypes == NULL) {
        free(c_sendtypes);
        err =  MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < size; ++i) {
            c_sendtypes[i] = PMPI_Type_f2c(sendtypes[i]);
            c_recvtypes[i] = PMPI_Type_f2c(recvtypes[i]);
        }
        err = MPI_Ineighbor_alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm, &c_request);
        if (MPI_SUCCESS == err) {
            *request = PMPI_Request_c2f(c_request);
        }
        free(c_sendtypes);
        free(c_recvtypes);
    }
    return err;
}

void MPI_INEIGHBOR_ALLTOALLW(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

void mpi_ineighbor_alltoallw(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

void mpi_ineighbor_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

void mpi_ineighbor_alltoallw__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ineighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm, request);
}

/* MPI_INIT Fortran wrappers */
static int geopm_mpi_init_f()
{
    int argc = 0;
    char** argv = NULL;
    return MPI_Init(&argc, &argv);
}

void MPI_INIT(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_f();
}

void mpi_init(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_f();
}

void mpi_init_(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_f();
}

void mpi_init__(MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_f();
}

/* MPI_INIT_THREAD Fortran wrappers */
static int geopm_mpi_init_thread_f(MPI_Fint *required, MPI_Fint *provided)
{
    int argc = 0;
    char** argv = NULL;
    return MPI_Init_thread(&argc, &argv, *required, provided);
}

void MPI_INIT_THREAD(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_thread_f(required, provided);
}

void mpi_init_thread(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_thread_f(required, provided);
}

void mpi_init_thread_(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_thread_f(required, provided);
}

void mpi_init_thread__(MPI_Fint *required, MPI_Fint *provided, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_init_thread_f(required, provided);
}

/* MPI_INTERCOMM_CREATE Fortran wrappers */
static int geopm_mpi_intercomm_create_f(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *bridge_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm)
{
    MPI_Comm c_newintercomm;
    MPI_Comm c_bridge_comm = PMPI_Comm_f2c(*bridge_comm);
    MPI_Comm c_local_comm = PMPI_Comm_f2c(*local_comm);
    int err = MPI_Intercomm_create(c_local_comm, *local_leader, c_bridge_comm, *remote_leader, *tag, &c_newintercomm);
    if (MPI_SUCCESS == err) {
        *newintercomm = PMPI_Comm_c2f(c_newintercomm);
    }
    return err;
}

void MPI_INTERCOMM_CREATE(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *bridge_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_create_f(local_comm, local_leader, bridge_comm, remote_leader, tag, newintercomm);
}

void mpi_intercomm_create(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *bridge_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_create_f(local_comm, local_leader, bridge_comm, remote_leader, tag, newintercomm);
}

void mpi_intercomm_create_(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *bridge_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_create_f(local_comm, local_leader, bridge_comm, remote_leader, tag, newintercomm);
}

void mpi_intercomm_create__(MPI_Fint *local_comm, MPI_Fint *local_leader, MPI_Fint *bridge_comm, MPI_Fint *remote_leader, MPI_Fint *tag, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_create_f(local_comm, local_leader, bridge_comm, remote_leader, tag, newintercomm);
}

/* MPI_INTERCOMM_MERGE Fortran wrappers */
static int geopm_mpi_intercomm_merge_f(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintercomm)
{
    MPI_Comm c_newintercomm;
    MPI_Comm c_intercomm = PMPI_Comm_f2c(*intercomm);
    int err = MPI_Intercomm_merge(c_intercomm, *high, &c_newintercomm);
    if (MPI_SUCCESS == err) {
        *newintercomm = PMPI_Comm_c2f(c_newintercomm);
    }
    return err;
}

void MPI_INTERCOMM_MERGE(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_merge_f(intercomm, high, newintercomm);
}

void mpi_intercomm_merge(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_merge_f(intercomm, high, newintercomm);
}

void mpi_intercomm_merge_(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_merge_f(intercomm, high, newintercomm);
}

void mpi_intercomm_merge__(MPI_Fint *intercomm, MPI_Fint *high, MPI_Fint *newintercomm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_intercomm_merge_f(intercomm, high, newintercomm);
}

/* MPI_IPROBE Fortran wrappers */
static int geopm_mpi_iprobe_f(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Iprobe(*source, *tag, c_comm, flag, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
        PMPI_Status_c2f(&c_status, status);
    }
    *flag = c_int_to_f_logical(flag);
    return err;
}

void MPI_IPROBE(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_iprobe_f(source, tag, comm, flag, status);
}

void mpi_iprobe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_iprobe_f(source, tag, comm, flag, status);
}

void mpi_iprobe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_iprobe_f(source, tag, comm, flag, status);
}

void mpi_iprobe__(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *flag, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_iprobe_f(source, tag, comm, flag, status);
}

/* MPI_IRECV Fortran wrappers */
static int geopm_mpi_irecv_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Irecv(buf, *count, c_datatype, *source, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IRECV(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irecv_f(buf, count, datatype, source, tag, comm, request);
}

void mpi_irecv(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irecv_f(buf, count, datatype, source, tag, comm, request);
}

void mpi_irecv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irecv_f(buf, count, datatype, source, tag, comm, request);
}

void mpi_irecv__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irecv_f(buf, count, datatype, source, tag, comm, request);
}

/* MPI_IREDUCE Fortran wrappers */
static int geopm_mpi_ireduce_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    int err = MPI_Ireduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IREDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_f(sendbuf, recvbuf, count, datatype, op, root, comm, request);
}

void mpi_ireduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_f(sendbuf, recvbuf, count, datatype, op, root, comm, request);
}

void mpi_ireduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_f(sendbuf, recvbuf, count, datatype, op, root, comm, request);
}

void mpi_ireduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_f(sendbuf, recvbuf, count, datatype, op, root, comm, request);
}

/* MPI_IREDUCE_SCATTER_BLOCK Fortran wrappers */
static int geopm_mpi_ireduce_scatter_block_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    int err = MPI_Ireduce_scatter_block(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IREDUCE_SCATTER_BLOCK(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm, request);
}

void mpi_ireduce_scatter_block(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm, request);
}

void mpi_ireduce_scatter_block_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm, request);
}

void mpi_ireduce_scatter_block__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm, request);
}

/* MPI_IREDUCE_SCATTER Fortran wrappers */
static int geopm_mpi_ireduce_scatter_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    int err = MPI_Ireduce_scatter(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_IREDUCE_SCATTER(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
}

void mpi_ireduce_scatter(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
}

void mpi_ireduce_scatter_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
}

void mpi_ireduce_scatter__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_ireduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm, request);
}

/* MPI_IRSEND Fortran wrappers */
static int geopm_mpi_irsend_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Irsend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }

    return err;
}

void MPI_IRSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irsend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_irsend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irsend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_irsend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irsend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_irsend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_irsend_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_ISCAN Fortran wrappers */
static int geopm_mpi_iscan_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    int err = MPI_Iscan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_ISCAN(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iscan(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iscan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

void mpi_iscan__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscan_f(sendbuf, recvbuf, count, datatype, op, comm, request);
}

/* MPI_ISCATTER Fortran wrappers */
static int geopm_mpi_iscatter_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Iscatter(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_ISCATTER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_iscatter(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_iscatter_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_iscatter__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

/* MPI_ISCATTERV Fortran wrappers */
static int geopm_mpi_iscatterv_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Request c_request;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Iscatterv(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_ISCATTERV(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_iscatterv(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_iscatterv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

void mpi_iscatterv__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_iscatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, request);
}

/* MPI_ISEND Fortran wrappers */
static int geopm_mpi_isend_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Isend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_ISEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_isend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_isend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_isend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_isend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_isend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_isend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_isend_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_ISSEND Fortran wrappers */
static int geopm_mpi_issend_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Issend(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_ISSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_issend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_issend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_issend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_issend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_issend_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_issend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_issend_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_MPROBE Fortran wrappers */
static int geopm_mpi_mprobe_f(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Message c_message;
    int err = MPI_Mprobe(*source, *tag, c_comm, &c_message, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
        PMPI_Status_c2f(&c_status, status);
        *message = MPI_Message_c2f(c_message);
    }
    return err;
}

void MPI_MPROBE(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_mprobe_f(source, tag, comm, message, status);
}

void mpi_mprobe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_mprobe_f(source, tag, comm, message, status);
}

void mpi_mprobe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_mprobe_f(source, tag, comm, message, status);
}

void mpi_mprobe__(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *message, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_mprobe_f(source, tag, comm, message, status);
}

/* MPI_NEIGHBOR_ALLGATHER Fortran wrappers */
static int geopm_mpi_neighbor_allgather_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Neighbor_allgather(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

void MPI_NEIGHBOR_ALLGATHER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_neighbor_allgather(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_neighbor_allgather_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_neighbor_allgather__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgather_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

/* MPI_NEIGHBOR_ALLGATHERV Fortran wrappers */
static int geopm_mpi_neighbor_allgatherv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Neighbor_allgatherv(sendbuf, *sendcount, c_sendtype, recvbuf, recvcounts, displs, c_recvtype, c_comm);
}

void MPI_NEIGHBOR_ALLGATHERV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

void mpi_neighbor_allgatherv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

void mpi_neighbor_allgatherv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

void mpi_neighbor_allgatherv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_allgatherv_f(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm);
}

/* MPI_NEIGHBOR_ALLTOALL Fortran wrappers */
static int geopm_mpi_neighbor_alltoall_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Neighbor_alltoall(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, c_comm);
}

void MPI_NEIGHBOR_ALLTOALL(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_neighbor_alltoall(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_neighbor_alltoall_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

void mpi_neighbor_alltoall__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoall_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
}

/* MPI_NEIGHBOR_ALLTOALLV Fortran wrappers */
static int geopm_mpi_neighbor_alltoallv_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Neighbor_alltoallv(sendbuf, sendcounts, sdispls, c_sendtype, recvbuf, recvcounts, rdispls, c_recvtype, c_comm);
}

void MPI_NEIGHBOR_ALLTOALLV(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

void mpi_neighbor_alltoallv(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

void mpi_neighbor_alltoallv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

void mpi_neighbor_alltoallv__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *sdispls, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *rdispls, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallv_f(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, rdispls, recvtype, comm);
}

/* MPI_NEIGHBOR_ALLTOALLW Fortran wrappers */
static int geopm_mpi_neighbor_alltoallw_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm)
{
    int size;
    int err = 0;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    PMPI_Comm_size(c_comm, &size);
    MPI_Datatype *c_sendtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    MPI_Datatype *c_recvtypes = (MPI_Datatype *)malloc(size * sizeof(MPI_Datatype));
    if (c_sendtypes == NULL) {
        err =  MPI_ERR_OTHER;
    }
    if (!err && c_recvtypes == NULL) {
        free(c_sendtypes);
        err =  MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < size; ++i) {
            c_sendtypes[i] = PMPI_Type_f2c(sendtypes[i]);
            c_recvtypes[i] = PMPI_Type_f2c(recvtypes[i]);
        }
        err = MPI_Neighbor_alltoallw(sendbuf, sendcounts, sdispls, c_sendtypes, recvbuf, recvcounts, rdispls, c_recvtypes, c_comm);
        free(c_sendtypes);
        free(c_recvtypes);
    }
    return err;
}

void MPI_NEIGHBOR_ALLTOALLW(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

void mpi_neighbor_alltoallw(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

void mpi_neighbor_alltoallw_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

void mpi_neighbor_alltoallw__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Aint *sdispls, MPI_Fint *sendtypes, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Aint *rdispls, MPI_Fint *recvtypes, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_neighbor_alltoallw_f(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, rdispls, recvtypes, comm);
}

/* MPI_PACK Fortran wrappers */
static int geopm_mpi_pack_f(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Pack(inbuf, *incount, c_datatype, outbuf, *outsize, position, c_comm);
}

void MPI_PACK(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_pack_f(inbuf, incount, datatype, outbuf, outsize, position, comm);
}

void mpi_pack(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_pack_f(inbuf, incount, datatype, outbuf, outsize, position, comm);
}

void mpi_pack_(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_pack_f(inbuf, incount, datatype, outbuf, outsize, position, comm);
}

void mpi_pack__(MPI_Fint *inbuf, MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *outbuf, MPI_Fint *outsize, MPI_Fint *position, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_pack_f(inbuf, incount, datatype, outbuf, outsize, position, comm);
}

/* MPI_PACK_SIZE Fortran wrappers */
static int geopm_mpi_pack_size_f(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Pack_size(*incount, c_datatype, c_comm, size);
}

void MPI_PACK_SIZE(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_pack_size_f(incount, datatype, comm, size);
}

void mpi_pack_size(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_pack_size_f(incount, datatype, comm, size);
}

void mpi_pack_size_(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_pack_size_f(incount, datatype, comm, size);
}

void mpi_pack_size__(MPI_Fint *incount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *size, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_pack_size_f(incount, datatype, comm, size);
}

/* MPI_PROBE Fortran wrappers */
static int geopm_mpi_probe_f(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Probe(*source, *tag, c_comm, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
        PMPI_Status_c2f(&c_status, status);
    }
    return err;
}

void MPI_PROBE(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_probe_f(source, tag, comm, status);
}

void mpi_probe(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_probe_f(source, tag, comm, status);
}

void mpi_probe_(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_probe_f(source, tag, comm, status);
}

void mpi_probe__(MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_probe_f(source, tag, comm, status);
}

/* MPI_RECV_INIT Fortran wrappers */
static int geopm_mpi_recv_init_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Recv_init(buf, *count, c_datatype, *source, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_RECV_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_recv_init_f(buf, count, datatype, source, tag, comm, request);
}

void mpi_recv_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_recv_init_f(buf, count, datatype, source, tag, comm, request);
}

void mpi_recv_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_recv_init_f(buf, count, datatype, source, tag, comm, request);
}

void mpi_recv_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_recv_init_f(buf, count, datatype, source, tag, comm, request);
}

/* MPI_RECV Fortran wrappers */
static int geopm_mpi_recv_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Recv(buf, *count, c_datatype, *source, *tag, c_comm, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
        PMPI_Status_c2f(&c_status, status);
    }
    return err;
}

void MPI_RECV(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_recv_f(buf, count, datatype, source, tag, comm, status);
}

void mpi_recv(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_recv_f(buf, count, datatype, source, tag, comm, status);
}

void mpi_recv_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_recv_f(buf, count, datatype, source, tag, comm, status);
}

void mpi_recv__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *source, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (buf == (void*)MPIR_F_MPI_BOTTOM) {
        buf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_recv_f(buf, count, datatype, source, tag, comm, status);
}

/* MPI_REDUCE Fortran wrappers */
static int geopm_mpi_reduce_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    return MPI_Reduce(sendbuf, recvbuf, *count, c_datatype, c_op, *root, c_comm);
}

void MPI_REDUCE(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_f(sendbuf, recvbuf, count, datatype, op, root, comm);
}

void mpi_reduce(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_f(sendbuf, recvbuf, count, datatype, op, root, comm);
}

void mpi_reduce_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_f(sendbuf, recvbuf, count, datatype, op, root, comm);
}

void mpi_reduce__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_f(sendbuf, recvbuf, count, datatype, op, root, comm);
}

/* MPI_REDUCE_SCATTER_BLOCK Fortran wrappers */
static int geopm_mpi_reduce_scatter_block_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    return MPI_Reduce_scatter_block(sendbuf, recvbuf, *recvcount, c_datatype, c_op, c_comm);
}

void MPI_REDUCE_SCATTER_BLOCK(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

void mpi_reduce_scatter_block(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

void mpi_reduce_scatter_block_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

void mpi_reduce_scatter_block__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_block_f(sendbuf, recvbuf, recvcount, datatype, op, comm);
}

/* MPI_REDUCE_SCATTER Fortran wrappers */
static int geopm_mpi_reduce_scatter_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    return MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, c_datatype, c_op, c_comm);
}

void MPI_REDUCE_SCATTER(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

void mpi_reduce_scatter(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

void mpi_reduce_scatter_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

void mpi_reduce_scatter__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *recvcounts, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_reduce_scatter_f(sendbuf, recvbuf, recvcounts, datatype, op, comm);
}

/* MPI_RSEND Fortran wrappers */
static int geopm_mpi_rsend_f(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Rsend(ibuf, *count, c_datatype, *dest, *tag, c_comm);
}

void MPI_RSEND(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_f(ibuf, count, datatype, dest, tag, comm);
}

void mpi_rsend(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_f(ibuf, count, datatype, dest, tag, comm);
}

void mpi_rsend_(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_f(ibuf, count, datatype, dest, tag, comm);
}

void mpi_rsend__(MPI_Fint *ibuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_f(ibuf, count, datatype, dest, tag, comm);
}

/* MPI_RSEND_INIT Fortran wrappers */
static int geopm_mpi_rsend_init_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Rsend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_RSEND_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_rsend_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_rsend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_rsend_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_rsend_init_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_SCAN Fortran wrappers */
static int geopm_mpi_scan_f(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Op c_op = PMPI_Op_f2c(*op);
    return MPI_Scan(sendbuf, recvbuf, *count, c_datatype, c_op, c_comm);
}

void MPI_SCAN(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_scan(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_scan_(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

void mpi_scan__(MPI_Fint *sendbuf, MPI_Fint *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scan_f(sendbuf, recvbuf, count, datatype, op, comm);
}

/* MPI_SCATTER Fortran wrappers */
static int geopm_mpi_scatter_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Scatter(sendbuf, *sendcount, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

void MPI_SCATTER(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_scatter(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_scatter_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_scatter__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatter_f(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

/* MPI_SCATTERV Fortran wrappers */
static int geopm_mpi_scatterv_f(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm)
{
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Scatterv(sendbuf, sendcounts, displs, c_sendtype, recvbuf, *recvcount, c_recvtype, *root, c_comm);
}

void MPI_SCATTERV(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_scatterv(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_scatterv_(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

void mpi_scatterv__(MPI_Fint *sendbuf, MPI_Fint *sendcounts, MPI_Fint *displs, MPI_Fint *sendtype, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_scatterv_f(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

/* MPI_SEND Fortran wrappers */
static int geopm_mpi_send_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Send(buf, *count, c_datatype, *dest, *tag, c_comm);
}

void MPI_SEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_f(buf, count, datatype, dest, tag, comm);
}

void mpi_send(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_f(buf, count, datatype, dest, tag, comm);
}

void mpi_send_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_f(buf, count, datatype, dest, tag, comm);
}

void mpi_send__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_f(buf, count, datatype, dest, tag, comm);
}

/* MPI_SEND_INIT Fortran wrappers */
static int geopm_mpi_send_init_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Send_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_SEND_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_send_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_send_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_send_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_send_init_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_SENDRECV Fortran wrappers */
static int geopm_mpi_sendrecv_f(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Datatype c_sendtype = PMPI_Type_f2c(*sendtype);
    MPI_Datatype c_recvtype = PMPI_Type_f2c(*recvtype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Sendrecv(sendbuf, *sendcount, c_sendtype, *dest, *sendtag, recvbuf, *recvcount, c_recvtype, *source, *recvtag, c_comm, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
        PMPI_Status_c2f(&c_status, status);
    }
    return err;
}

void MPI_SENDRECV(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_sendrecv_f(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

void mpi_sendrecv(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_sendrecv_f(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

void mpi_sendrecv_(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_sendrecv_f(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

void mpi_sendrecv__(MPI_Fint *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *recvbuf, MPI_Fint *recvcount, MPI_Fint *recvtype, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    if (sendbuf == (void*)MPIR_F_MPI_IN_PLACE) {
        sendbuf = MPI_IN_PLACE;
    }
    if (sendbuf == (void*)MPIR_F_MPI_BOTTOM) {
        sendbuf = MPI_BOTTOM;
    }
    if (recvbuf == (void*)MPIR_F_MPI_BOTTOM) {
        recvbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_sendrecv_f(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status);
}

/* MPI_SENDRECV_REPLACE Fortran wrappers */
static int geopm_mpi_sendrecv_replace_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Sendrecv_replace(buf, *count, c_datatype, *dest, *sendtag, *source, *recvtag, c_comm, &c_status);
    if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
        PMPI_Status_c2f(&c_status, status);
    }
    return err;
}

void MPI_SENDRECV_REPLACE(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_sendrecv_replace_f(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

void mpi_sendrecv_replace(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_sendrecv_replace_f(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

void mpi_sendrecv_replace_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_sendrecv_replace_f(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

void mpi_sendrecv_replace__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *sendtag, MPI_Fint *source, MPI_Fint *recvtag, MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_sendrecv_replace_f(buf, count, datatype, dest, sendtag, source, recvtag, comm, status);
}

/* MPI_SSEND Fortran wrappers */
static int geopm_mpi_ssend_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    return MPI_Ssend(buf, *count, c_datatype, *dest, *tag, c_comm);
}

void MPI_SSEND(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_f(buf, count, datatype, dest, tag, comm);
}

void mpi_ssend(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_f(buf, count, datatype, dest, tag, comm);
}

void mpi_ssend_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_f(buf, count, datatype, dest, tag, comm);
}

void mpi_ssend__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_f(buf, count, datatype, dest, tag, comm);
}

/* MPI_SSEND_INIT Fortran wrappers */
static int geopm_mpi_ssend_init_f(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request)
{
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    MPI_Request c_request;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Ssend_init(buf, *count, c_datatype, *dest, *tag, c_comm, &c_request);
    if (MPI_SUCCESS == err) {
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_SSEND_INIT(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_ssend_init(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_ssend_init_(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_init_f(buf, count, datatype, dest, tag, comm, request);
}

void mpi_ssend_init__(MPI_Fint *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *dest, MPI_Fint *tag, MPI_Fint *comm, MPI_Fint *request, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_ssend_init_f(buf, count, datatype, dest, tag, comm, request);
}

/* MPI_TOPO_TEST Fortran wrappers */
static int geopm_mpi_topo_test_f(MPI_Fint *comm, MPI_Fint *status)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Topo_test(c_comm, status);
    return err;
}

void MPI_TOPO_TEST(MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_topo_test_f(comm, status);
}

void mpi_topo_test(MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_topo_test_f(comm, status);
}

void mpi_topo_test_(MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_topo_test_f(comm, status);
}

void mpi_topo_test__(MPI_Fint *comm, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_topo_test_f(comm, status);
}

/* MPI_UNPACK Fortran wrappers */
static int geopm_mpi_unpack_f(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm)
{
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    return MPI_Unpack(inbuf, *insize, position, outbuf, *outcount, c_datatype, c_comm);
}

void MPI_UNPACK(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_unpack_f(inbuf, insize, position, outbuf, outcount, datatype, comm);
}

void mpi_unpack(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_unpack_f(inbuf, insize, position, outbuf, outcount, datatype, comm);
}

void mpi_unpack_(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_unpack_f(inbuf, insize, position, outbuf, outcount, datatype, comm);
}

void mpi_unpack__(MPI_Fint *inbuf, MPI_Fint *insize, MPI_Fint *position, MPI_Fint *outbuf, MPI_Fint *outcount, MPI_Fint *datatype, MPI_Fint *comm, MPI_Fint *ierr)
{
    if (inbuf == (void*)MPIR_F_MPI_BOTTOM) {
        inbuf = MPI_BOTTOM;
    }
    if (outbuf == (void*)MPIR_F_MPI_BOTTOM) {
        outbuf = MPI_BOTTOM;
    }
    *ierr =  geopm_mpi_unpack_f(inbuf, insize, position, outbuf, outcount, datatype, comm);
}

/* MPI_WAITALL Fortran wrappers */
static int geopm_mpi_waitall_f(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses)
{
    int err = 0;
    MPI_Request *c_array_of_requests = (MPI_Request *)malloc(*count * sizeof(MPI_Request));
    MPI_Status *c_array_of_statuses = (MPI_Status *)malloc(*count * sizeof(MPI_Status));
    if (c_array_of_requests == NULL) {
        err =  MPI_ERR_OTHER;
    }
    if (!err && c_array_of_statuses == NULL) {
        free(c_array_of_requests);
        err =  MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < *count; ++i) {
            c_array_of_requests[i] = PMPI_Request_f2c(array_of_requests[i]);
        }
        err = MPI_Waitall(*count, c_array_of_requests, c_array_of_statuses);

        if (MPI_SUCCESS == err && (array_of_statuses != MPI_F_STATUSES_IGNORE)) {
            for (int i = 0; i < *count; ++i) {
                PMPI_Status_c2f(&c_array_of_statuses[i], &array_of_statuses[i * (sizeof(MPI_Status) / sizeof(int))]);
                array_of_requests[i] = PMPI_Request_c2f(c_array_of_requests[i]);
            }
        }
        free(c_array_of_requests);
        free(c_array_of_statuses);
    }
    return err;
}

void MPI_WAITALL(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitall_f(count, array_of_requests, array_of_statuses);
}

void mpi_waitall(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitall_f(count, array_of_requests, array_of_statuses);
}

void mpi_waitall_(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitall_f(count, array_of_requests, array_of_statuses);
}

void mpi_waitall__(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitall_f(count, array_of_requests, array_of_statuses);
}

/* MPI_WAITANY Fortran wrappers */
static int geopm_mpi_waitany_f(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status)
{
    int err = 0;
    MPI_Status c_status;
    MPI_Request *c_array_of_requests = (MPI_Request *)malloc(*count * sizeof(MPI_Request));
    if (c_array_of_requests == NULL) {
        err = MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < *count; ++i) {
            c_array_of_requests[i] = PMPI_Request_f2c(array_of_requests[i]);
        }
        err = MPI_Waitany(*count, c_array_of_requests, index, &c_status);
        if (MPI_SUCCESS == err && status != MPI_F_STATUS_IGNORE) {
            if (*index != MPI_UNDEFINED) {
                array_of_requests[*index] = PMPI_Request_c2f(c_array_of_requests[*index]);
                ++(*index);
            }
            PMPI_Status_c2f(&c_status, status);
        }
        free(c_array_of_requests);
    }
    return err;
}

void MPI_WAITANY(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitany_f(count, array_of_requests, index, status);
}

void mpi_waitany(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitany_f(count, array_of_requests, index, status);
}

void mpi_waitany_(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitany_f(count, array_of_requests, index, status);
}

void mpi_waitany__(MPI_Fint *count, MPI_Fint *array_of_requests, MPI_Fint *index, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitany_f(count, array_of_requests, index, status);
}

/* MPI_WAIT Fortran wrappers */
static int geopm_mpi_wait_f(MPI_Fint *request, MPI_Fint *status)
{
    MPI_Status c_status;
    MPI_Request c_request = PMPI_Request_f2c(*request);
    int err = MPI_Wait(&c_request, &c_status);
    if (MPI_SUCCESS == err && (status != MPI_F_STATUS_IGNORE ||
                               status != MPI_F_STATUS_IGNORE ||
                               status != MPI_F_STATUS_IGNORE ||
                               status != MPI_F_STATUS_IGNORE)) {
        PMPI_Status_c2f(&c_status, status);
        *request = PMPI_Request_c2f(c_request);
    }
    return err;
}

void MPI_WAIT(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_wait_f(request, status);
}

void mpi_wait(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_wait_f(request, status);
}

void mpi_wait_(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_wait_f(request, status);
}

void mpi_wait__(MPI_Fint *request, MPI_Fint *status, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_wait_f(request, status);
}

/* MPI_WAITSOME Fortran wrappers */
static int geopm_mpi_waitsome_f(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses)
{
    int err = 0;
    MPI_Request *c_array_of_requests = (MPI_Request *)malloc(*incount * sizeof(MPI_Request));
    MPI_Status *c_array_of_statuses = (MPI_Status *)malloc(*incount * sizeof(MPI_Status));
    if (c_array_of_requests == NULL) {
        err =  MPI_ERR_OTHER;
    }
    if (!err && c_array_of_statuses == NULL) {
        free(c_array_of_requests);
        err =  MPI_ERR_OTHER;
    }
    if (!err) {
        for (int i = 0; i < *incount; ++i) {
            c_array_of_requests[i] = PMPI_Request_f2c(array_of_requests[i]);
        }
        int err = MPI_Waitsome(*incount, c_array_of_requests, outcount, array_of_indices, c_array_of_statuses);
        if (MPI_SUCCESS == err) {
            if (MPI_UNDEFINED != *outcount) {
                for (int i = 0; i < *outcount; ++i) {
                    array_of_requests[array_of_indices[i]] = PMPI_Request_c2f(c_array_of_requests[array_of_indices[i]]);
                    ++array_of_indices[i];
                }
            }
            if (array_of_statuses != MPI_F_STATUSES_IGNORE) {
                for (int i = 0; i < *incount; ++i) {
                    PMPI_Status_c2f(&c_array_of_statuses[i], &array_of_statuses[i * (sizeof(MPI_Status) / sizeof(int))]);
                }
            }
        }
        free(c_array_of_requests);
        free(c_array_of_statuses);
    }
    return err;
}

void MPI_WAITSOME(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitsome_f(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

void mpi_waitsome(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitsome_f(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

void mpi_waitsome_(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitsome_f(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

void mpi_waitsome__(MPI_Fint *incount, MPI_Fint *array_of_requests, MPI_Fint *outcount, MPI_Fint *array_of_indices, MPI_Fint *array_of_statuses, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_waitsome_f(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
}

/* MPI_WIN_ALLOCATE Fortran wrappers */
static int geopm_mpi_win_allocate_f(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Win c_win;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Win_allocate(*size, *disp_unit, c_info, c_comm, baseptr, &c_win);
    if (MPI_SUCCESS == err) {
        *win = PMPI_Win_c2f(c_win);
    }
    return err;
}

void MPI_WIN_ALLOCATE(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_f(size, disp_unit, info, comm, baseptr, win);
}

void mpi_win_allocate(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_f(size, disp_unit, info, comm, baseptr, win);
}

void mpi_win_allocate_(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_f(size, disp_unit, info, comm, baseptr, win);
}

void mpi_win_allocate__(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_f(size, disp_unit, info, comm, baseptr, win);
}

/* MPI_WIN_ALLOCATE_SHARED Fortran wrappers */
static int geopm_mpi_win_allocate_shared_f(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Win c_win;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Win_allocate_shared(*size, *disp_unit, c_info, c_comm, baseptr, &c_win);
    if (MPI_SUCCESS == err) {
        *win = PMPI_Win_c2f(c_win);
    }
    return err;
}

void MPI_WIN_ALLOCATE_SHARED(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_shared_f(size, disp_unit, info, comm, baseptr, win);
}

void mpi_win_allocate_shared(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_shared_f(size, disp_unit, info, comm, baseptr, win);
}

void mpi_win_allocate_shared_(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_shared_f(size, disp_unit, info, comm, baseptr, win);
}

void mpi_win_allocate_shared__(MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *baseptr, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_allocate_shared_f(size, disp_unit, info, comm, baseptr, win);
}

/* MPI_WIN_CREATE_DYNAMIC Fortran wrappers */
static int geopm_mpi_win_create_dynamic_f(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Win c_win;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Win_create_dynamic(c_info, c_comm, &c_win);
    if (MPI_SUCCESS == err) {
        *win = PMPI_Win_c2f(c_win);
    }
    return err;
}

void MPI_WIN_CREATE_DYNAMIC(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_dynamic_f(info, comm, win);
}

void mpi_win_create_dynamic(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_dynamic_f(info, comm, win);
}

void mpi_win_create_dynamic_(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_dynamic_f(info, comm, win);
}

void mpi_win_create_dynamic__(MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_dynamic_f(info, comm, win);
}

/* MPI_WIN_CREATE Fortran wrappers */
static int geopm_mpi_win_create_f(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win)
{
    MPI_Info c_info = PMPI_Info_f2c(*info);
    MPI_Win c_win;
    MPI_Comm c_comm = PMPI_Comm_f2c(*comm);
    int err = MPI_Win_create(base, *size, *disp_unit, c_info, c_comm, &c_win);
    if (MPI_SUCCESS == err) {
        *win = PMPI_Win_c2f(c_win);
    }
    return err;
}

void MPI_WIN_CREATE(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_f(base, size, disp_unit, info, comm, win);
}

void mpi_win_create(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_f(base, size, disp_unit, info, comm, win);
}

void mpi_win_create_(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_f(base, size, disp_unit, info, comm, win);
}

void mpi_win_create__(MPI_Fint *base, MPI_Fint *size, MPI_Fint *disp_unit, MPI_Fint *info, MPI_Fint *comm, MPI_Fint *win, MPI_Fint *ierr)
{
    *ierr =  geopm_mpi_win_create_f(base, size, disp_unit, info, comm, win);
}
#endif //GEOPM_OMPI_BOTTOM
