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
#include <mpi.h>

#include "geopm.h"
#include "geopm_ctl.h"
#include "geopm_policy.h"
#include "config.h"

int geopm_ctl_spawn(struct geopm_ctl_c *ctl)
{
#if 0
    /* FIX ME use ctl structure properly*/
    int err = 0, i;
    int *ctl_err = NULL;
    const char *geopmctl_path_default = "geopmctl";
    int world_size, world_rank, shm_rank, ctl_size, is_shm_root;
    MPI_Comm shm_comm, split_comm;
    MPI_Info info = MPI_INFO_NULL;

    if (!geopmctl_path) {
        geopmctl_path = geopmctl_path_default;
    }
    err = geopm_comm_num_node(app_comm, &ctl_size);
    if (!err) {
        ctl_err = (int *)calloc(ctl_size, sizeof(int));
        if (ctl_err == NULL) {
            err = ENOMEM;
        }
    }
    if (!err) {
        err = MPI_Info_create(&info);
    }
    if (!err) {
        err = MPI_Info_set(info, "pernode", "true");
    }
    if (!err) {
        // FIXME need to construct argv from control and report
        err = MPI_Comm_spawn((char *)geopmctl_path, NULL, ctl_size, info, 0, app_comm, ctl_intercomm, ctl_err);
    }
    for (i = 0; i < ctl_size; ++i) {
        if (ctl_err[i] != MPI_SUCCESS) {
            err = ctl_err[i];
            break;
        }
    }
    if (ctl_err) {
        free(ctl_err);
    }
    if (info != MPI_INFO_NULL) {
        MPI_Info_free(&info);
    }
    return err;
#endif
    return 0;
}
