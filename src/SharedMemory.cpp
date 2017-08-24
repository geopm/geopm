
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

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <iostream>
#include <sstream>

#include "geopm_time.h"
#include "SharedMemory.hpp"
#include "Exception.hpp"
#include "geopm_signal_handler.h"
#include "config.h"

namespace geopm
{
    SharedMemory::SharedMemory(const std::string &shm_key, size_t size)
        : m_shm_key(shm_key)
        , m_size(size)
    {
        if (!size) {
            throw Exception("SharedMemory: Cannot create shared memory region of zero size",  GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        mode_t old_mask = umask(0);
        int shm_id = shm_open(m_shm_key.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP| S_IWGRP | S_IROTH| S_IWOTH);
        if (shm_id < 0) {
            std::ostringstream ex_str;
            ex_str << "SharedMemory: Could not open shared memory with key " << m_shm_key;
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int err = ftruncate(shm_id, size);
        if (err) {
            (void) close(shm_id);
            (void) shm_unlink(m_shm_key.c_str());
            (void) umask(old_mask);
            std::ostringstream ex_str;
            ex_str << "SharedMemory: Could not extend shared memory to size "  << size;
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
        if (m_ptr == MAP_FAILED) {
            (void) close(shm_id);
            (void) shm_unlink(m_shm_key.c_str());
            (void) umask(old_mask);
            throw Exception("SharedMemory: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = close(shm_id);
        if (err) {
            (void) umask(old_mask);
            throw Exception("SharedMemory: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        umask(old_mask);
    }

    SharedMemory::~SharedMemory()
    {
        if (munmap(m_ptr, m_size)) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: " << Exception("SharedMemory: Could not unmap pointer",
                                                  errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__).what() << std::endl;
#endif
        }
        (void) shm_unlink(m_shm_key.c_str());
    }

    void *SharedMemory::pointer(void)
    {
        return m_ptr;
    }

    std::string SharedMemory::key(void)
    {
        return m_shm_key;
    }

    size_t SharedMemory::size(void)
    {
        return m_size;
    }

    SharedMemoryUser::SharedMemoryUser(const std::string &shm_key)
        : SharedMemoryUser(shm_key, INT_MAX)
    {

    }

    SharedMemoryUser::SharedMemoryUser(const std::string &shm_key, unsigned int timeout)
        : m_shm_key(shm_key)
        , m_size(0)
        , m_is_linked(false)
    {
        int shm_id = -1;
        struct stat stat_struct;
        int err = 0;

        if (!timeout) {
            shm_id = shm_open(shm_key.c_str(), O_RDWR, 0);
            if (shm_id < 0) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryUser: Could not open shared memory with key \""  <<  shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = fstat(shm_id, &stat_struct);
            if (err) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryUser: fstat() error on shared memory with key \"" << shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_size = stat_struct.st_size;

            m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
            if (m_ptr == MAP_FAILED) {
                (void) close(shm_id);
                throw Exception("SharedMemoryUser: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        else {
            struct geopm_time_s begin_time;
            struct geopm_time_s curr_time;

            geopm_time(&begin_time);
            curr_time = begin_time;
            while (shm_id < 0 && geopm_time_diff(&begin_time, &curr_time) < (double)timeout) {
                geopm_signal_handler_check();
                shm_id = shm_open(shm_key.c_str(), O_RDWR, 0);
                geopm_time(&curr_time);
            }
            if (shm_id < 0) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryUser: Could not open shared memory with key \"" << shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            while (!m_size && geopm_time_diff(&begin_time, &curr_time) < (double)timeout) {
                geopm_signal_handler_check();
                err = fstat(shm_id, &stat_struct);
                if (!err) {
                    m_size = stat_struct.st_size;
                }
                geopm_time(&curr_time);
            }
            if (!m_size) {
                (void) close(shm_id);
                throw Exception("SharedMemoryUser: Opened shared memory region, but it is zero length", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
            if (m_ptr == MAP_FAILED) {
                (void) close(shm_id);
                throw Exception("SharedMemoryUser: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        err = close(shm_id);
        if (err) {
            throw Exception("SharedMemoryUser: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_is_linked = true;
    }

    SharedMemoryUser::~SharedMemoryUser()
    {
        if (munmap(m_ptr, m_size)) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: " << Exception("SharedMemory: Could not unmap pointer",
                                                  errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__).what() << std::endl;
#endif
        }
    }

    void *SharedMemoryUser::pointer(void)
    {
        return m_ptr;
    }

    std::string SharedMemoryUser::key(void)
    {
        return m_shm_key;
    }

    size_t SharedMemoryUser::size(void)
    {
        return m_size;
    }

    void SharedMemoryUser::unlink(void)
    {
        if (m_is_linked) {
            int err = shm_unlink(m_shm_key.c_str());
            if (err) {
                std::ostringstream tmp_str;
                tmp_str << "SharedMemoryUser::unlink() Call to shm_unlink(" << m_shm_key  << ") failed",
                throw Exception(tmp_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_is_linked = false;
        }
    }
}
