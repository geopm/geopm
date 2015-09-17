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

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <system_error>
#include <string.h>

#include "PolicyController.hpp"


int geopm_policy_controller_create(char *shm_key, struct geopm_policy_message_s initial_policy, struct geopm_policy_controller_c **policy_controller)
{
    int err = 0;
    geopm::PolicyController *polctl = NULL;

    try {
        polctl = new geopm::PolicyController(std::string(shm_key), initial_policy);
    }
    catch (std::exception ex) {
        err = 1;
    }
    if (!err) {
        *policy_controller = (struct geopm_policy_controller_c *)polctl;
    }
    return err;
}

int geopm_policy_controller_destroy(geopm_policy_controller_c *policy_controller)
{
    int err = 0;
    geopm::PolicyController *polctl = (geopm::PolicyController *)policy_controller;
    try {
        delete polctl;
    }
    catch (std::exception ex) {
        err = 1;
    }
    return err;
}

int geopm_policy_controller_set(void *policy_controller, struct geopm_policy_message_s policy)
{
    int err = 0;
    geopm::PolicyController *polctl = (geopm::PolicyController *)policy_controller;
    try {
        polctl->set_policy(policy);
    }
    catch (std::exception ex) {
        err = 1;
    }
    return err;
}

namespace geopm
{
    PolicyController::PolicyController(std::string shm_key, struct geopm_policy_message_s initial_policy)
        : m_shm_key(shm_key)
    {
        int err, shm_id;
        pthread_mutexattr_t mutex_attr;

        shm_id = shm_open(m_shm_key.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG);
        if (shm_id < 0) {
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Could not open shared memory region for policy control.\n");
        }
        err = ftruncate(shm_id, sizeof(struct geopm_policy_shmem_s));
        if (err) {
            (void) shm_unlink(m_shm_key.c_str());
            (void) close(shm_id);
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Could not extend shared memory region with ftruncate for policy control.\n");
        }
        m_policy_shmem = (struct geopm_policy_shmem_s *) mmap(NULL, sizeof(struct geopm_policy_shmem_s),
                         PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
        if (m_policy_shmem == MAP_FAILED) {
            (void) shm_unlink(m_shm_key.c_str());
            (void) close(shm_id);
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Could not map shared memory region for policy control.\n");
        }

        err = close(shm_id);
        if (err) {
            (void) munmap(m_policy_shmem, sizeof(struct geopm_policy_shmem_s));
            (void) shm_unlink(m_shm_key.c_str());
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Could not close shared memory file descriptor for policy control.\n");
        }
        m_policy_shmem->policy = initial_policy;

        memset(&mutex_attr, 0, sizeof(mutex_attr));
        err = pthread_mutexattr_init(&mutex_attr);
        if (err) {
            (void) munmap(m_policy_shmem, sizeof(struct geopm_policy_shmem_s));
            (void) shm_unlink(m_shm_key.c_str());
            throw std::system_error(std::error_code(err, std::system_category()),
                                    "Could not initialize mutex attributes for policy control.\n");
        }

        err = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
        if (err) {
            (void) munmap(m_policy_shmem, sizeof(struct geopm_policy_shmem_s));
            (void) shm_unlink(m_shm_key.c_str());
            throw std::system_error(std::error_code(err, std::system_category()),
                                    "Could not set mutex attributes for policy control.\n");
        }

        err = pthread_mutex_init(&(m_policy_shmem->lock), &mutex_attr);
        if (err) {
            (void) munmap(m_policy_shmem, sizeof(struct geopm_policy_shmem_s));
            (void) shm_unlink(m_shm_key.c_str());
            throw std::system_error(std::error_code(err, std::system_category()),
                                    "Could not initialize mutex for policy control.\n");
        }
        m_policy_shmem->is_init = true;
    }

    PolicyController::~PolicyController()
    {
        int err;
        err = munmap(m_policy_shmem, sizeof(struct geopm_policy_shmem_s));
        if (err) {
            (void) shm_unlink(m_shm_key.c_str());
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Could not munmap shared memory region on PolicyControl destruction.\n");
        }
        err = shm_unlink(m_shm_key.c_str());
        if (err) {
            throw std::system_error(std::error_code(err, std::system_category()),
                                    "Could not unlink shared memory region on PolicyControl destruction.\n");
        }
    }

    void PolicyController::set_policy(struct geopm_policy_message_s policy)
    {
        int err;
        err = pthread_mutex_lock(&(m_policy_shmem->lock));
        if (err) {
            throw std::system_error(std::error_code(err, std::system_category()),
                                    "Could not lock shared memory region policy control.\n");
        }
        m_policy_shmem->policy = policy;
        err = pthread_mutex_unlock(&(m_policy_shmem->lock));
        if (err) {
            throw std::system_error(std::error_code(err, std::system_category()),
                                    "Could not unlock shared memory region policy control.\n");
        }
    }
}
