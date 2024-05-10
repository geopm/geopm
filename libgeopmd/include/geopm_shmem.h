/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef GEOPM_SHMEM_H_INCLUDE
#define GEOPM_SHMEM_H_INCLUDE

#include "geopm_public.h"

#ifdef __cplusplus
#include <string>

namespace geopm
{
    std::string GEOPM_PUBLIC
        shmem_path_prof(const std::string &shm_key, int pid, int uid);
    void GEOPM_PUBLIC
        shmem_create_prof(const std::string &shm_key, size_t size, int pid, int uid, int gid);
}

extern "C" {
#endif

int GEOPM_PUBLIC
    geopm_shmem_path_prof(const char *shm_key, int pid, int uid,
                          size_t shm_path_max, char *shm_path);
int GEOPM_PUBLIC
    geopm_shmem_create_prof(const char *shm_key, size_t size, int pid, int uid, int gid);

#ifdef __cplusplus
}
#endif
#endif
