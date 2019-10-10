/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#include "SharedMemoryImp.hpp"

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
#include "geopm_signal_handler.h"
#include "Exception.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    /// @brief Size of the lock in memory.
    static constexpr size_t M_LOCK_SIZE = sizeof(pthread_mutex_t);

    static void setup_mutex(pthread_mutex_t *lock)
    {
        pthread_mutexattr_t lock_attr;
        int err = pthread_mutexattr_init(&lock_attr);
        if (err) {
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_ERRORCHECK);
        if (err) {
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
        if (err) {
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutex_init(lock, &lock_attr);
        if (err) {
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::unique_ptr<SharedMemory> SharedMemory::make_unique(const std::string &shm_key, size_t size)
    {
        return geopm::make_unique<SharedMemoryImp>(shm_key, size);
    }

    std::shared_ptr<SharedMemory> SharedMemory::make_shared(const std::string &shm_key, size_t size)
    {
        return std::make_shared<SharedMemoryImp>(shm_key, size);
    }

    std::unique_ptr<SharedMemoryUser> SharedMemoryUser::make_unique(const std::string &shm_key, unsigned int timeout)
    {
        return geopm::make_unique<SharedMemoryUserImp>(shm_key, timeout);
    }

    std::shared_ptr<SharedMemoryUser> SharedMemoryUser::make_shared(const std::string &shm_key, unsigned int timeout)
    {
        return std::make_shared<SharedMemoryUserImp>(shm_key, timeout);
    }

    SharedMemoryImp::SharedMemoryImp(const std::string &shm_key, size_t size)
        : m_shm_key(shm_key)
        , m_size(size + M_LOCK_SIZE)
    {
        if (!size) {
            throw Exception("SharedMemoryImp: Cannot create shared memory region of zero size", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        mode_t old_mask = umask(0);
        int shm_id = shm_open(m_shm_key.c_str(), O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP| S_IWGRP | S_IROTH| S_IWOTH);
        if (shm_id < 0) {
            std::ostringstream ex_str;
            ex_str << "SharedMemoryImp: Could not open shared memory with key " << m_shm_key;
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int err = ftruncate(shm_id, m_size);
        if (err) {
            (void) close(shm_id);
            (void) shm_unlink(m_shm_key.c_str());
            (void) umask(old_mask);
            std::ostringstream ex_str;
            ex_str << "SharedMemoryImp: Could not extend shared memory to size " << m_size;
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
        if (m_ptr == MAP_FAILED) {
            (void) close(shm_id);
            (void) shm_unlink(m_shm_key.c_str());
            (void) umask(old_mask);
            throw Exception("SharedMemoryImp: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = close(shm_id);
        if (err) {
            (void) umask(old_mask);
            throw Exception("SharedMemoryImp: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        umask(old_mask);

        setup_mutex((pthread_mutex_t*)m_ptr);
    }

    SharedMemoryImp::~SharedMemoryImp()
    {
        if (m_ptr != nullptr) {
            if (munmap(m_ptr, m_size)) {
#ifdef GEOPM_DEBUG
                std::cerr << "Warning: <geopm> SharedMemoryImp: Could not unmap pointer" << std::endl;
#endif
            }
        }
    }

    void SharedMemoryImp::unlink(void)
    {
        int err = shm_unlink(m_shm_key.c_str());
        if (err) {
            std::ostringstream tmp_str;
            tmp_str << "SharedMemoryImp::unlink() Call to shm_unlink(" << m_shm_key << ") failed";
            throw Exception(tmp_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    void *SharedMemoryImp::pointer(void) const
    {
        return (char*)m_ptr + M_LOCK_SIZE;
    }

    std::string SharedMemoryImp::key(void) const
    {
        return m_shm_key;
    }

    size_t SharedMemoryImp::size(void) const
    {
        return m_size - M_LOCK_SIZE;
    }

    std::unique_ptr<SharedMemoryScopedLock> SharedMemoryImp::get_scoped_lock(void)
    {
        return geopm::make_unique<SharedMemoryScopedLock>((pthread_mutex_t*)m_ptr);
    }


    SharedMemoryUserImp::SharedMemoryUserImp(const std::string &shm_key, unsigned int timeout)
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
                ex_str << "SharedMemoryUserImp: Could not open shared memory with key \""  <<  shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = fstat(shm_id, &stat_struct);
            if (err) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryUserImp: fstat() error on shared memory with key \"" << shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_size = stat_struct.st_size;

            m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
            if (m_ptr == MAP_FAILED) {
                (void) close(shm_id);
                throw Exception("SharedMemoryUserImp: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        else {
            struct geopm_time_s begin_time;
            geopm_time(&begin_time);
            while (shm_id < 0 && geopm_time_since(&begin_time) < (double)timeout) {
                geopm_signal_handler_check();
                shm_id = shm_open(shm_key.c_str(), O_RDWR, 0);
            }
            if (shm_id < 0) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryUserImp: Could not open shared memory with key \"" << shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            while (!m_size && geopm_time_since(&begin_time) < (double)timeout) {
                geopm_signal_handler_check();
                err = fstat(shm_id, &stat_struct);
                if (!err) {
                    m_size = stat_struct.st_size;
                }
            }
            if (!m_size) {
                (void) close(shm_id);
                throw Exception("SharedMemoryUserImp: Opened shared memory region, but it is zero length", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
            if (m_ptr == MAP_FAILED) {
                (void) close(shm_id);
                throw Exception("SharedMemoryUserImp: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        err = close(shm_id);
        if (err) {
            throw Exception("SharedMemoryUserImp: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_is_linked = true;
    }

    SharedMemoryUserImp::~SharedMemoryUserImp()
    {
        if (munmap(m_ptr, m_size)) {
#ifdef GEOPM_DEBUG
            std::cerr << "Warning: <geopm> SharedMemoryUserImp: Could not unmap pointer" << std::endl;
#endif
        }
    }

    void *SharedMemoryUserImp::pointer(void) const
    {
        return (char*)m_ptr + M_LOCK_SIZE;
    }

    std::string SharedMemoryUserImp::key(void) const
    {
        return m_shm_key;
    }

    size_t SharedMemoryUserImp::size(void) const
    {
        return m_size - M_LOCK_SIZE;
    }

    void SharedMemoryUserImp::unlink(void)
    {
        if (m_is_linked) {
            int err = shm_unlink(m_shm_key.c_str());
            if (err) {
                std::ostringstream tmp_str;
                tmp_str << "SharedMemoryUserImp::unlink() Call to shm_unlink(" << m_shm_key << ") failed";
                throw Exception(tmp_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_is_linked = false;
        }
    }

    std::unique_ptr<SharedMemoryScopedLock> SharedMemoryUserImp::get_scoped_lock(void)
    {
        return geopm::make_unique<SharedMemoryScopedLock>((pthread_mutex_t*)m_ptr);
    }
}
