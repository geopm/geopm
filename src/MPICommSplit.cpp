/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sys/stat.h>
#include <unistd.h>
#include <sstream>

#include "geopm_mpi_comm_split.h"
#include "geopm/SharedMemory.hpp"
#include "Environment.hpp"
#include "geopm/Exception.hpp"
#include "Controller.hpp"
#include "Comm.hpp"
#include "MPIComm.hpp"
#include "ApplicationSampler.hpp"

#include "config.h"


extern "C"
{
    int geopm_ctl_create(MPI_Comm comm, struct geopm_ctl_c **ctl)
    {
        int err = 0;
        try {
            auto tmp_comm = std::unique_ptr<geopm::Comm>(new geopm::MPIComm(comm));
            *ctl = (struct geopm_ctl_c *)(new geopm::Controller(std::move(tmp_comm)));
        }
        catch (...) {
            err = geopm::exception_handler(std::current_exception(), true);
        }
        return err;
    }

    int geopm_ctl_create_f(int comm, struct geopm_ctl_c **ctl)
    {
        return geopm_ctl_create(MPI_Comm_f2c(comm), ctl);
    }

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
            std::ostringstream shmem_path;
            shmem_path << "/dev/shm/geopm-shm-" << geteuid() << "-comm-split-" << tag;
            std::shared_ptr<geopm::SharedMemory> shmem = nullptr;
            std::shared_ptr<geopm::SharedMemory> shmem_user = nullptr;
            int rank, color = -1;

            MPI_Comm_rank(comm, &rank);
            // remove shared memory file if one already exists
            (void)unlink(shmem_path.str().c_str());
            MPI_Barrier(comm);
            err = stat(shmem_path.str().c_str(), &stat_struct);
            if (!err || (err && errno != ENOENT)) {
                std::stringstream ex_str;
                ex_str << "geopm_comm_split_shared(): " << shmem_path.str()
                       << " already exists and cannot be deleted.";
                throw geopm::Exception(ex_str.str(), GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            MPI_Barrier(comm);
            try {
                shmem = geopm::SharedMemory::make_unique_owner(shmem_path.str(), sizeof(int));
            }
            catch (const geopm::Exception &ex) {
                if (ex.err_value() != EEXIST) {
                    throw ex;
                }
            }
            if (!shmem) {
                shmem_user = geopm::SharedMemory::make_unique_user(shmem_path.str(), geopm::environment().timeout());
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
            if (shmem) {
                shmem->unlink();
            }
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
