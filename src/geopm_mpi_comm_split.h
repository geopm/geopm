/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_MPI_COMM_SPLIT_H_INCLUDE
#define GEOPM_MPI_COMM_SPLIT_H_INCLUDE

#ifndef GEOPM_TEST
#include <mpi.h>
#else
typedef int MPI_Comm;
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*****************/
/* MPI COMM APIS */
/*****************/
int geopm_comm_split_ppn1(MPI_Comm comm, const char *tag, MPI_Comm *ppn1_comm);

int geopm_comm_split_shared(MPI_Comm comm, const char *tag, MPI_Comm *split_comm);

int geopm_comm_split(MPI_Comm comm, const char *tag, MPI_Comm *split_comm, int *is_ctl_comm);
#ifdef __cplusplus
}

#endif
#endif
