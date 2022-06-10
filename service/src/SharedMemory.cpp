/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <utility>

#include "geopm_time.h"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "config.h"

namespace geopm
{

    /// @brief Size of the lock in memory.
    static constexpr size_t M_LOCK_SIZE = geopm::hardware_destructive_interference_size;
    static_assert(sizeof(pthread_mutex_t) <= M_LOCK_SIZE, "M_LOCK_SIZE not large enough for mutex type");

    static void setup_mutex(pthread_mutex_t *lock)
    {
        pthread_mutexattr_t lock_attr;
        int err = pthread_mutexattr_init(&lock_attr);
        if (err) {
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_settype(&lock_attr, PTHREAD_MUTEX_ERRORCHECK);
        if (err) {
            (void) pthread_mutexattr_destroy(&lock_attr);
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
        if (err) {
            (void) pthread_mutexattr_destroy(&lock_attr);
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutex_init(lock, &lock_attr);
        if (err) {
            (void) pthread_mutexattr_destroy(&lock_attr);
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        err = pthread_mutexattr_destroy(&lock_attr);
        if (err) {
            throw Exception("SharedMemory::setup_mutex(): pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::unique_ptr<SharedMemory> SharedMemory::make_unique_owner(const std::string &shm_key, size_t size)
    {
        std::unique_ptr<SharedMemoryImp> owner = geopm::make_unique<SharedMemoryImp>();
        owner->create_memory_region(shm_key, size, false);
        return std::unique_ptr<SharedMemory>(std::move(owner));
    }

    std::unique_ptr<SharedMemory> SharedMemory::make_unique_owner_secure(const std::string &shm_key, size_t size)
    {
        std::unique_ptr<SharedMemoryImp> owner = geopm::make_unique<SharedMemoryImp>();
        owner->create_memory_region(shm_key, size, true);
        return std::unique_ptr<SharedMemory>(std::move(owner));
    }

    std::unique_ptr<SharedMemory> SharedMemory::make_unique_user(const std::string &shm_key, unsigned int timeout)
    {
        std::unique_ptr<SharedMemoryImp> user = geopm::make_unique<SharedMemoryImp>();
        user->attach_memory_region(shm_key, timeout);
        return std::unique_ptr<SharedMemory>(std::move(user));
    }

    SharedMemoryImp::SharedMemoryImp()
        : m_size(0)
        , m_ptr(NULL)
        , m_is_linked(false)
        , m_do_unlink_check(false)
    {

    }

    void SharedMemoryImp::create_memory_region(const std::string &shm_key, size_t size, bool is_secure)
    {
        if (!size) {
            throw Exception("SharedMemoryImp: Cannot create shared memory region of zero size", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_shm_key = shm_key;
        m_shm_path = construct_shm_path(shm_key);
        m_size = size + M_LOCK_SIZE;

        mode_t old_mask = umask(0);
        int shm_id = 0;
        if (is_secure) {
            shm_id = open(m_shm_path.c_str(), O_RDWR | O_CREAT | O_EXCL,
                          S_IRUSR | S_IWUSR);
        }
        else {
            shm_id = open(m_shm_path.c_str(), O_RDWR | O_CREAT | O_EXCL,
                          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                          S_IROTH | S_IWOTH);
        }
        if (shm_id < 0) {
            std::ostringstream ex_str;
            ex_str << "SharedMemoryImp: Could not open shared memory with key " << m_shm_key;
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        int err = ftruncate(shm_id, m_size);
        if (err) {
            (void) close(shm_id);
            (void) ::unlink(m_shm_path.c_str());
            (void) umask(old_mask);
            std::ostringstream ex_str;
            ex_str << "SharedMemoryImp: Could not extend shared memory to size " << m_size;
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
        if (m_ptr == MAP_FAILED) {
            (void) close(shm_id);
            (void) ::unlink(m_shm_path.c_str());
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

        m_is_linked = true;
        m_do_unlink_check = false;
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
        // ProfileSampler destructor calls unlink, so don't throw if constructed
        // as owner
        if (m_is_linked) {
            int err = ::unlink(m_shm_path.c_str());
            if (err && m_do_unlink_check) {
                std::ostringstream tmp_str;
                tmp_str << "SharedMemoryImp::unlink() Call to unlink(" << m_shm_path  << ") failed";
                throw Exception(tmp_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_is_linked = false;
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

    void SharedMemoryImp::attach_memory_region(const std::string &shm_key, unsigned int timeout)
    {
        m_shm_key = shm_key;
        m_shm_path = construct_shm_path(shm_key);
        m_is_linked = false;

        int shm_id = -1;
        struct stat stat_struct;
        int err = 0;

        if (!timeout) {
            shm_id = open(m_shm_path.c_str(), O_RDWR, 0);
            if (shm_id < 0) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryImp: Could not open shared memory with key \""  <<  shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = fstat(shm_id, &stat_struct);
            if (err) {
                (void) close(shm_id);
                std::ostringstream ex_str;
                ex_str << "SharedMemoryImp: fstat() error on shared memory with key \"" << shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_size = stat_struct.st_size;

            m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
            if (m_ptr == MAP_FAILED) {
                (void) close(shm_id);
                throw Exception("SharedMemoryImp: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        else {
            struct geopm_time_s begin_time;
            geopm_time(&begin_time);
            while (shm_id < 0 && geopm_time_since(&begin_time) < (double)timeout) {
                shm_id = open(m_shm_path.c_str(), O_RDWR, 0);
            }
            if (shm_id < 0) {
                std::ostringstream ex_str;
                ex_str << "SharedMemoryImp: Could not open shared memory with key \"" << shm_key << "\"";
                throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            while (!m_size && geopm_time_since(&begin_time) < (double)timeout) {
                err = fstat(shm_id, &stat_struct);
                if (!err) {
                    m_size = stat_struct.st_size;
                }
            }
            if (!m_size) {
                (void) close(shm_id);
                throw Exception("SharedMemoryImp: Opened shared memory region, but it is zero length", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            m_ptr = mmap(NULL, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
            if (m_ptr == MAP_FAILED) {
                (void) close(shm_id);
                throw Exception("SharedMemoryImp: Could not mmap shared memory region", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }

        err = close(shm_id);
        if (err) {
            throw Exception("SharedMemoryImp: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        m_is_linked = true;
        m_do_unlink_check = true;
    }

    void SharedMemoryImp::chown(const unsigned int uid, const unsigned int gid) const {
        int err = 0;
        int shm_id = -1;
        if (m_is_linked) {
            shm_id = open(m_shm_path.c_str(), O_RDWR, 0);
        }
        else {
            throw Exception("SharedMemoryImp: Cannot chown shm that has been unlinked.", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        if (shm_id < 0) {
            std::ostringstream ex_str;
            ex_str << "SharedMemoryImp: Could not open shared memory with key \""  <<  m_shm_key << "\"";
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        err = fchown(shm_id, uid, gid);
        if (err) {
            (void) close(shm_id);
            std::ostringstream ex_str;
            ex_str << "SharedMemoryImp: Could not chown shmem with key (" << m_shm_key
                   << ") to UID (" << std::to_string(uid)
                   << "), GID (" << std::to_string(gid) + ")";
            throw Exception(ex_str.str(), errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        err = close(shm_id);
        if (err) {
            throw Exception("SharedMemoryImp: Could not close shared memory file", errno ? errno : GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    std::string SharedMemoryImp::construct_shm_path(const std::string &key)
    {
        std::string path;
        if (key.length() > 1 && key[0] == '/' &&
            key.find('/', 1) == std::string::npos) {
            // shmem key

            // pam_systemd/logind enabled?
            std::string usr_run_dir = "/run/user/" +
                                      std::to_string(getuid());
            if (access(usr_run_dir.c_str(), F_OK) == 0) {
                path = usr_run_dir + key;
            }
            else {
                path = "/dev/shm" + key;
            }
        } else {
            // regular file path
            path = key;
        }

        return path;
    }
}
