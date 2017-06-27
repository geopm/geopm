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

#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include "geopm_env.h"
#include "SharedMemory.hpp"
#include "geopm_mpi_comm.h"
#include "Exception.hpp"

extern "C"
{
static int geopm_comm_split_imp(MPI_Comm comm, const char *tag, int *num_node, MPI_Comm *split_comm, int *is_ctl_comm);

int geopm_comm_split_ppn1(MPI_Comm comm, const char *tag, MPI_Comm *ppn1_comm)
{
    int num_node = 0;
    int is_shm_root = 0;
    int err = geopm_comm_split_imp(comm, tag, &num_node, ppn1_comm, &is_shm_root);
    if (!err && !is_shm_root) {
        err = MPI_Comm_free(ppn1_comm);
        *ppn1_comm = MPI_COMM_NULL;
    }
    return err;
}

int geopm_comm_split_shared(MPI_Comm comm, const char *tag, MPI_Comm *split_comm)
{
    int err = 0;
    struct stat stat_struct;
    try {
        std::ostringstream shmem_key;
        shmem_key << geopm_env_shmkey() << "-comm-split-" << tag;
        std::ostringstream shmem_path;
        shmem_path << "/dev/shm" << shmem_key.str();
        geopm::SharedMemory *shmem = NULL;
        geopm::SharedMemoryUser *shmem_user = NULL;
        int rank, color = -1;

        MPI_Comm_rank(comm, &rank);
        // remove shared memory file if one already exists
        (void)unlink(shmem_path.str().c_str());
        MPI_Barrier(comm);
        err = stat(shmem_path.str().c_str(), &stat_struct);
        if (!err || (err && errno != ENOENT)) {
            std::stringstream ex_str;
            ex_str << "geopm_comm_split_shared(): " << shmem_key.str()
                << " already exists and cannot be deleted.";
            throw geopm::Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        MPI_Barrier(comm);
        try {
            shmem = new geopm::SharedMemory(shmem_key.str(), sizeof(int));
        }
        catch (geopm::Exception ex) {
            if (ex.err_value() != EEXIST) {
                throw ex;
            }
        }
        if (!shmem) {
            shmem_user = new geopm::SharedMemoryUser(shmem_key.str(), 1);
        }
        else {
            color = rank;
            *((int*)(shmem->pointer())) = color;
        }
        MPI_Barrier(comm);
        if (shmem_user) {
            color = *((int*)(shmem_user->pointer()));
        }
        err = MPI_Comm_split(comm, color, rank, split_comm);
        delete shmem;
        delete shmem_user;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}

int geopm_comm_split(MPI_Comm comm, const char *tag, MPI_Comm *split_comm, int *is_ctl_comm)
{
    int num_node = 0;
    return geopm_comm_split_imp(comm, tag, &num_node, split_comm, is_ctl_comm);
}

static int geopm_comm_split_imp(MPI_Comm comm, const char *tag, int *num_node, MPI_Comm *split_comm, int *is_shm_root)
{
    int err, comm_size, comm_rank, shm_rank;
    MPI_Comm shm_comm = MPI_COMM_NULL, tmp_comm = MPI_COMM_NULL;
    MPI_Comm *split_comm_ptr;

    *is_shm_root = 0;

    if (split_comm) {
        split_comm_ptr = split_comm;
    }
    else {
        split_comm_ptr = &tmp_comm;
    }

    err = MPI_Comm_size(comm, &comm_size);
    if (!err) {
        err = MPI_Comm_rank(comm, &comm_rank);
    }
    if (!err) {
        err = geopm_comm_split_shared(comm, tag, &shm_comm);
    }
    if (!err) {
        err = MPI_Comm_rank(shm_comm, &shm_rank);
    }
    if (!err) {
        if (!shm_rank) {
            *is_shm_root = 1;
        }
        else {
            *is_shm_root = 0;
        }
        err = MPI_Comm_split(comm, *is_shm_root, comm_rank, split_comm_ptr);
    }
    if (!err) {
        if (*is_shm_root == 1) {
            err = MPI_Comm_size(*split_comm_ptr, num_node);
        }
    }
    if (!err) {
        err = MPI_Bcast(num_node, 1, MPI_INT, 0, shm_comm);
    }
    if (shm_comm != MPI_COMM_NULL) {
        MPI_Comm_free(&shm_comm);
    }
    if (!split_comm) {
        MPI_Comm_free(split_comm_ptr);
    }
    return err;
}
}
