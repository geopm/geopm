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

#include <iostream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm_env.h"
#include "Exception.hpp"
#include "SharedMemory.hpp"

class SharedMemoryTest : public :: testing :: Test
{
    protected:
        void SetUp();
        void TearDown();
        void config_shmem();
        void config_shmem_u();
        void cleanup_shmem();
        void cleanup_shmem_u();
        std::string m_shm_key;
        size_t m_size;
        geopm::SharedMemory *m_shmem;
        geopm::SharedMemoryUser *m_shmem_u;
};

void SharedMemoryTest::SetUp()
{
    m_shm_key = std::string(geopm_env_shmkey()) + "-SharedMemoryTest-" + std::to_string(getpid());
    m_size = sizeof(size_t);
}

void SharedMemoryTest::TearDown()
{
}

void SharedMemoryTest::config_shmem()
{
    m_shmem = new geopm::SharedMemory(m_shm_key, m_size);
}

void SharedMemoryTest::config_shmem_u()
{
    m_shmem_u = new geopm::SharedMemoryUser(m_shm_key, 1); // 1 second timeout
    // if 1 second timeout constructor worked so should this, just throw away instance
    // warning, will spin for INT_MAX seconds if there is a failure...
    delete new geopm::SharedMemoryUser(m_shm_key);
}

void SharedMemoryTest::cleanup_shmem()
{
    if (m_shmem) {
        delete m_shmem;
        m_shmem = NULL;
    }
}

void SharedMemoryTest::cleanup_shmem_u()
{
    if (m_shmem_u) {
        m_shmem_u->unlink();
        delete m_shmem_u;
        m_shmem_u = NULL;
    }
}

TEST_F(SharedMemoryTest, invalid_construction)
{
    m_shm_key += "-invalid_construction";
    EXPECT_THROW((new geopm::SharedMemory(m_shm_key, 0)), geopm::Exception);  // invalid memory region size
    EXPECT_THROW((new geopm::SharedMemoryUser(m_shm_key, 1)), geopm::Exception);
    EXPECT_THROW((new geopm::SharedMemory("", m_size)), geopm::Exception);  // invalid key
    EXPECT_THROW((new geopm::SharedMemoryUser("", 1)), geopm::Exception);
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
    cleanup_shmem_u();
    cleanup_shmem();
}

TEST_F(SharedMemoryTest, share_data_ipc)
{
    m_shm_key += "-share_data_ipc";
    size_t shared_data = 0xDEADBEEFCAFED00D;

    pid_t pid = fork();
    if (pid) {
        // parent process
        config_shmem_u();
        EXPECT_EQ(memcmp(m_shmem_u->pointer(), &shared_data, m_size), 0);
        cleanup_shmem_u();
    } else {
        // child process
        config_shmem();
        memcpy(m_shmem->pointer(), &shared_data, m_size);
        sleep(1);
        cleanup_shmem();
        exit(0);
    }
}
