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

#include <sys/stat.h>

#include <future>
#include <chrono>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "SharedMemoryImp.hpp"
#include "geopm_test.hpp"

using geopm::SharedMemory;
using geopm::SharedMemoryImp;
using geopm::SharedMemoryUser;
using geopm::SharedMemoryUserImp;

class SharedMemoryTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void config_shmem();
        void config_shmem_u();
        std::string m_shm_key;
        size_t m_size;
        std::unique_ptr<SharedMemory> m_shmem;
        std::unique_ptr<SharedMemoryUser> m_shmem_u;
        int M_TIMEOUT;
};

void SharedMemoryTest::SetUp()
{
    m_shm_key = "/geopm-shm-foo-SharedMemoryTest-" + std::to_string(getpid());
    m_size = sizeof(size_t);
    m_shmem = NULL;
    m_shmem_u = NULL;
    M_TIMEOUT = 1;
}

void SharedMemoryTest::TearDown()
{
    if (m_shmem_u) {
        m_shmem_u->unlink();
    }
}

void SharedMemoryTest::config_shmem()
{
    m_shmem.reset(new geopm::SharedMemoryImp(m_shm_key, m_size, M_TIMEOUT));
}

void SharedMemoryTest::config_shmem_u()
{
    m_shmem_u.reset(new geopm::SharedMemoryUserImp(m_shm_key, M_TIMEOUT));
}

TEST_F(SharedMemoryTest, fd_check)
{
    struct stat buf;
    std::string key_path("/dev/shm");
    m_shm_key += "-fd_check";
    key_path += m_shm_key;

    config_shmem();
    sleep(5);
    EXPECT_EQ(stat(key_path.c_str(), &buf), 0) << "Something (likely systemd) is removing shmem entries after creation.\n"
                                               << "See https://superuser.com/a/1179962 for more information.";
    config_shmem_u();
    ASSERT_NE(nullptr, m_shmem_u);
    m_shmem_u->unlink();
    EXPECT_EQ(stat(key_path.c_str(), &buf), -1);
    EXPECT_EQ(errno, ENOENT);
}

TEST_F(SharedMemoryTest, invalid_construction)
{
    m_shm_key += "-invalid_construction";
    EXPECT_THROW((SharedMemoryImp(m_shm_key, 0, M_TIMEOUT)), geopm::Exception);  // invalid memory region size
    EXPECT_THROW((SharedMemoryUserImp(m_shm_key, M_TIMEOUT)), geopm::Exception);
    EXPECT_THROW((SharedMemoryImp("", m_size, M_TIMEOUT)), geopm::Exception);  // invalid key
    EXPECT_THROW((SharedMemoryUserImp("", M_TIMEOUT)), geopm::Exception);
}

TEST_F(SharedMemoryTest, share_data)
{
    m_shm_key += "-share_data";
    config_shmem();
    config_shmem_u();
    size_t shared_data = 0xDEADBEEFCAFED00D;
    void *alias1 = m_shmem->pointer();
    void *alias2 = m_shmem_u->pointer();
    memcpy(alias1, &shared_data, m_size);
    EXPECT_EQ(memcmp(alias1, &shared_data, m_size), 0);
    EXPECT_EQ(memcmp(alias2, &shared_data, m_size), 0);
}

TEST_F(SharedMemoryTest, share_data_ipc)
{
    m_shm_key += "-share_data_ipc";
    size_t shared_data = 0xDEADBEEFCAFED00D;

    pid_t pid = fork();
    if (pid) {
        // parent process
        config_shmem_u();
        sleep(1);
        EXPECT_EQ(memcmp(m_shmem_u->pointer(), &shared_data, m_size), 0);
    } else {
        // child process
        config_shmem();
        memcpy(m_shmem->pointer(), &shared_data, m_size);
        sleep(2);
        exit(0);
    }
}

TEST_F(SharedMemoryTest, lock_shmem)
{
    config_shmem();
    config_shmem_u();

    // mutex is hidden at address before the user memory region
    // normally, this mutex should not be accessed directly.  This test
    // checks that get_scoped_lock() has the expected side effects on the
    // mutex.
    pthread_mutex_t *mutex = (pthread_mutex_t*)((char*)m_shmem->pointer() - sizeof(pthread_mutex_t));

    // mutex starts out lockable
    EXPECT_EQ(0, pthread_mutex_trylock(mutex));
    EXPECT_EQ(0, pthread_mutex_unlock(mutex));

    auto lock = m_shmem->get_scoped_lock();
    // should not be able to lock
    EXPECT_NE(0, pthread_mutex_trylock(mutex));
    GEOPM_EXPECT_THROW_MESSAGE(m_shmem->get_scoped_lock(),
                               EDEADLK, "Resource deadlock avoided");

    // destroy the lock
    lock.reset();

    // mutex should be lockable again
    EXPECT_EQ(0, pthread_mutex_trylock(mutex));
    EXPECT_EQ(0, pthread_mutex_unlock(mutex));
}

TEST_F(SharedMemoryTest, lock_shmem_u)
{
    config_shmem();
    config_shmem_u();

    // mutex is hidden at address before the user memory region
    // normally, this mutex should not be accessed directly.  This test
    // checks that get_scoped_lock() has the expected side effects on the
    // mutex.
    pthread_mutex_t *mutex = (pthread_mutex_t*)((char*)m_shmem_u->pointer() - sizeof(pthread_mutex_t));

    // mutex starts out lockable
    EXPECT_EQ(0, pthread_mutex_trylock(mutex));
    EXPECT_EQ(0, pthread_mutex_unlock(mutex));

    auto lock = m_shmem_u->get_scoped_lock();
    // should not be able to lock
    EXPECT_NE(0, pthread_mutex_trylock(mutex));
    GEOPM_EXPECT_THROW_MESSAGE(m_shmem_u->get_scoped_lock(),
                               EDEADLK, "Resource deadlock avoided");

    // destroy the lock
    lock.reset();

    // mutex should be lockable again
    EXPECT_EQ(0, pthread_mutex_trylock(mutex));
    EXPECT_EQ(0, pthread_mutex_unlock(mutex));
}

TEST_F(SharedMemoryTest, lock_shmem_timeout)
{
    config_shmem();
    config_shmem_u();

    // mutex is hidden at address before the user memory region
    // normally, this mutex should not be accessed directly.  This test
    // checks that get_scoped_lock() has the expected side effects on the
    // mutex.
    pthread_mutex_t *mutex = (pthread_mutex_t*)((char*)m_shmem->pointer() - sizeof(pthread_mutex_t));

    // other side locks mutex and dies
    //EXPECT_EQ(0, pthread_mutex_lock(mutex));
    auto other = std::async(std::launch::async,
                            [mutex] {
                                EXPECT_EQ(0, pthread_mutex_lock(mutex));
                            });
    other.wait();

    // get_scoped_lock should throw after timeout
    GEOPM_EXPECT_THROW_MESSAGE(m_shmem->get_scoped_lock(),
                               ETIMEDOUT, "timed out");
}

TEST_F(SharedMemoryTest, lock_shmem_u_timeout)
{
    config_shmem();
    config_shmem_u();

    // mutex is hidden at address before the user memory region
    // normally, this mutex should not be accessed directly.  This test
    // checks that get_scoped_lock() has the expected side effects on the
    // mutex.
    pthread_mutex_t *mutex = (pthread_mutex_t*)((char*)m_shmem_u->pointer() - sizeof(pthread_mutex_t));

    // other side locks mutex and dies
    //EXPECT_EQ(0, pthread_mutex_lock(mutex));
    auto other = std::async(std::launch::async,
                            [mutex] {
                                EXPECT_EQ(0, pthread_mutex_lock(mutex));
                            });
    other.wait();

    // get_scoped_lock should throw after timeout
    GEOPM_EXPECT_THROW_MESSAGE(m_shmem_u->get_scoped_lock(),
                               ETIMEDOUT, "timed out");
}
