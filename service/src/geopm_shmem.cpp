/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <sstream>
#include <cstring>
#include <unistd.h>

#include "geopm/SharedMemory.hpp"
#include "geopm/Exception.hpp"
#include "geopm_shmem.h"

namespace geopm
{
    std::string shmem_path_prof(const std::string &shm_key, int pid, int uid)
    {
        std::ostringstream result;
        int id = pid;
        if (shm_key == "status") {
            // The status key is a shared resource
            id = uid;
        }
        result << "/run/geopm-service/profile-" << id << "-" << shm_key;
        return result.str();
    }

    void shmem_create_prof(const std::string &shm_key, size_t size, int pid, int uid, int gid)
    {
        std::string shm_path = shmem_path_prof(shm_key, pid, uid);
        auto shm = SharedMemory::make_unique_owner_secure(shm_path, size);
        shm->chown(uid, gid);
    }
}

int geopm_shmem_create_prof(const char *shm_key, size_t size, int pid, int uid, int gid)
{
    int err = 0;
    try {
        geopm::shmem_create_prof(shm_key, size, pid, uid, gid);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_shmem_path_prof(const char *shm_key, int pid, int uid, size_t shm_path_max, char *shm_path)
{
    int err = 0;
    try {
        std::string result = geopm::shmem_path_prof(shm_key, pid, uid);
        if (result.size() >= shm_path_max) {
            throw geopm::Exception("geopm_shmem_path(): shm_path_max is too small to store result: " + result,
                                   GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        strncpy(shm_path, result.c_str(), shm_path_max);
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}
