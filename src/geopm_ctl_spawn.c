/*
 * Copyright (c) 2015, Intel Corporation
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

#include <mpi.h>


int geopm_ctl_spawn(MPI_Comm app_comm, const char *geopmctl_path, MPI_Comm *ctl_intercomm)
{
    int err = 0, i;
    int *ctl_err;
    const char *geopmctl_path_default = "geopmctl";
    int world_size, world_rank, shm_rank, ctl_size, is_shm_root;
    MPI_Comm shm_comm, split_comm;
    MPI_Info info;

    if (!geopmctl_path) {
        geopmctl_path = geopmctl_path_default;
    }

    MPI_Comm_size(app_comm, &world_size);
    MPI_Comm_rank(app_comm, &world_rank);
    MPI_Comm_split_type(app_comm, MPI_COMM_TYPE_SHARED, world_rank, MPI_INFO_NULL, &shm_comm);
    MPI_Comm_rank(shm_comm, &shm_rank);
    if (!shm_rank) {
        is_shm_root = 1;
    }
    else {
        is_shm_root = 0;
    }
    MPI_Comm_split(app_comm, is_shm_root, world_rank, &split_comm);
    if (is_shm_root == 1) {
        MPI_Comm_size(split_comm, &ctl_size);
    }
    MPI_Bcast(&ctl_size, 1, MPI_INT, 0, shm_comm);
    ctl_err = (int *)calloc(ctl_size, sizeof(int));
    MPI_Info_create(&info);
    MPI_Info_set(info, "pernode", "true");
    MPI_Comm_spawn((char *)geopmctl_path, NULL, ctl_size, info, 0, app_comm, ctl_intercomm, ctl_err);
    for (i = 0; i < ctl_size; ++i) {
        if (ctl_err[i] != MPI_SUCCESS) {
            err = ctl_err[i];
            break;
        }
    }
    free(ctl_err);
    // FIXME Destroy info, shm_comm, and split_comm
    // FIXME Check return codes
    return err;
}
