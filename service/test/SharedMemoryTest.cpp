/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "geopm/Exception.hpp"
#include "geopm/SharedMemory.hpp"
#include "SharedMemoryImp.hpp"
#include "geopm/Helper.hpp"
#include "geopm_test.hpp"

using geopm::SharedMemory;
using geopm::SharedMemoryImp;
using testing::Throw;

class SharedMemoryTest : public ::testing::Test, public SharedMemoryImp
{
    protected:
        void SetUp();
        void TearDown();
        void config_shmem(const std::string &shm_key);
        void config_shmem_s(const std::string &shm_key);
        void config_shmem_u(const std::string &shm_key);

        void fd_check_test(const std::string &shm_key,
                           const std::string &key_path);
        void share_data_test(const std::string &shm_key);
        void share_data_ipc_test(const std::string &shm_key);
        void lock_shmem_test(const std::string &shm_key);
        void lock_shmem_u_test(const std::string &shm_key);
        void chown_test(const std::string &shm_key);
        void default_permissions_test(const std::string &shm_key,
                                      const std::string &key_path);
        void secure_permissions_test(const std::string &shm_key,
                                     const std::string &key_path);

        size_t m_size;
        std::unique_ptr<SharedMemory> m_shmem;
        std::unique_ptr<SharedMemory> m_shmem_u;

        std::string m_key_shm;
        std::string m_key_file;
};

void SharedMemoryTest::SetUp()
{
    m_key_shm = "/geopm-shm-foo-SharedMemoryTest-" + std::to_string(getpid());
    m_key_file = "/tmp/geopm-shm-foo-SharedMemoryTest-" +
                 std::to_string(getpid());
    m_size = sizeof(size_t);
    m_shmem = nullptr;
    m_shmem_u = nullptr;
}

void SharedMemoryTest::TearDown()
{
    if (m_shmem_u) {
        m_shmem_u->unlink();
    }
}

void SharedMemoryTest::config_shmem(const std::string &shm_key)
{
    m_shmem = SharedMemory::make_unique_owner(shm_key, m_size);
}

void SharedMemoryTest::config_shmem_s(const std::string &shm_key)
{
    m_shmem = SharedMemory::make_unique_owner_secure(shm_key, m_size);
}

void SharedMemoryTest::config_shmem_u(const std::string &shm_key)
{
    m_shmem_u = SharedMemory::make_unique_user(shm_key, 1); // 1 second timeout
}

void SharedMemoryTest::fd_check_test(const std::string &shm_key,
                                     const std::string &key_path)
{
    struct stat buf;
    std::string key = shm_key + "-fd_check";
    std::string path = key_path + "-fd_check";

    config_shmem(key);
    sleep(5);
    EXPECT_EQ(stat(path.c_str(), &buf), 0)
        << "Something (likely systemd) is removing shmem entries after "
           "creation.\n"
        << "See https://superuser.com/a/1179962 for more information.";
    config_shmem_u(key);
    ASSERT_NE(nullptr, m_shmem_u);
    m_shmem_u->unlink();
    EXPECT_EQ(stat(path.c_str(), &buf), -1);
    EXPECT_EQ(errno, ENOENT);
}

TEST_F(SharedMemoryTest, fd_check_shm)
{
    std::string key_path = construct_shm_path(m_key_shm);
    fd_check_test(m_key_shm, key_path);
}

TEST_F(SharedMemoryTest, fd_check_file)
{
    std::string key_path = construct_shm_path(m_key_file);
    fd_check_test(m_key_file, key_path);
}

TEST_F(SharedMemoryTest, invalid_construction)
{
    std::string shm_key = m_key_shm + "-invalid_construction";
    EXPECT_THROW((SharedMemory::make_unique_owner(shm_key, 0)),
                 geopm::Exception);  // invalid memory region size
    EXPECT_THROW((SharedMemory::make_unique_user(shm_key, 1)),
                 geopm::Exception);
    EXPECT_THROW((SharedMemory::make_unique_owner("", m_size)),
                 geopm::Exception);  // invalid key
    EXPECT_THROW((SharedMemory::make_unique_user("", 1)),
                 geopm::Exception);
}

void SharedMemoryTest::share_data_test(const std::string &shm_key)
{
    std::string key = shm_key + "-share_data";
    config_shmem(key);
    config_shmem_u(key);
    size_t shared_data = 0xDEADBEEFCAFED00D;
    void *alias1 = m_shmem->pointer();
    void *alias2 = m_shmem_u->pointer();
    memcpy(alias1, &shared_data, m_size);
    EXPECT_EQ(memcmp(alias1, &shared_data, m_size), 0);
    EXPECT_EQ(memcmp(alias2, &shared_data, m_size), 0);
}

TEST_F(SharedMemoryTest, share_data_shm)
{
    share_data_test(m_key_shm);
}

TEST_F(SharedMemoryTest, share_data_file)
{
    share_data_test(m_key_file);
}

void SharedMemoryTest::share_data_ipc_test(const std::string &shm_key)
{
    std::string key = shm_key + "-share_data_ipc";
    size_t shared_data = 0xDEADBEEFCAFED00D;

    pid_t pid = fork();
    if (pid) {
        // parent process
        config_shmem_u(key);
        sleep(1);
        EXPECT_EQ(memcmp(m_shmem_u->pointer(), &shared_data, m_size), 0);
    } else {
        // child process
        config_shmem(key);
        memcpy(m_shmem->pointer(), &shared_data, m_size);
        sleep(2);
        exit(0);
    }
}

TEST_F(SharedMemoryTest, share_data_ipc_shm)
{
    share_data_ipc_test(m_key_shm);
}

TEST_F(SharedMemoryTest, share_data_ipc_file)
{
    share_data_ipc_test(m_key_file);
}

void SharedMemoryTest::lock_shmem_test(const std::string &shm_key)
{
    std::string key = shm_key;
    config_shmem(key);
    config_shmem_u(key);

    // mutex is hidden at address before the user memory region
    // normally, this mutex should not be accessed directly.  This test
    // checks that get_scoped_lock() has the expected side effects on the
    // mutex.
    pthread_mutex_t *mutex =
        (pthread_mutex_t*)((char*)m_shmem->pointer() -
                           geopm::hardware_destructive_interference_size);

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

TEST_F(SharedMemoryTest, lock_shmem_shm)
{
    lock_shmem_test(m_key_shm);
}

TEST_F(SharedMemoryTest, lock_shmem_file)
{
    lock_shmem_test(m_key_file);
}

void SharedMemoryTest::lock_shmem_u_test(const std::string &shm_key)
{
    std::string key = shm_key;
    config_shmem(key);
    config_shmem_u(key);

    // mutex is hidden at address before the user memory region
    // normally, this mutex should not be accessed directly.  This test
    // checks that get_scoped_lock() has the expected side effects on the
    // mutex.
    pthread_mutex_t *mutex =
        (pthread_mutex_t*)((char*)m_shmem_u->pointer() -
                           geopm::hardware_destructive_interference_size);

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

TEST_F(SharedMemoryTest, lock_shmem_u_shm)
{
    lock_shmem_u_test(m_key_shm);
}

TEST_F(SharedMemoryTest, lock_shmem_u_file)
{
    lock_shmem_u_test(m_key_file);
}

void SharedMemoryTest::chown_test(const std::string &shm_key)
{
    std::string key = shm_key;
    config_shmem(key);
    uid_t uid = getuid();
    gid_t gid = getgid();

    // Sanity check: set to my own gid/uid
    EXPECT_NO_THROW(m_shmem->chown(uid, gid));

    // Try to set root gid/uid
    GEOPM_EXPECT_THROW_MESSAGE(m_shmem->chown(0, 0),
                               EPERM, "Could not chown shmem with key");

    m_shmem->unlink(); // Manually unlink unless config_shmem_u() is called

    GEOPM_EXPECT_THROW_MESSAGE(m_shmem->chown(uid, gid),
                               GEOPM_ERROR_RUNTIME, "unlinked");
}

TEST_F(SharedMemoryTest, chown_shm)
{
    chown_test(m_key_shm);
}

TEST_F(SharedMemoryTest, chown_file)
{
    chown_test(m_key_file);
}

void SharedMemoryTest::default_permissions_test(const std::string &shm_key,
                                                const std::string &key_path)
{
    std::string key = shm_key;
    int fd = -1;
    uint32_t permissions_bits = 0;
    uint32_t expected_permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
                                    S_IROTH | S_IWOTH;
    struct stat stat_struct;

    config_shmem(key);
    fd = open(key_path.c_str(), O_RDWR, 0);
    fstat(fd, &stat_struct);
    // Mask off type bits, save mode bits
    permissions_bits = stat_struct.st_mode & ~(S_IFMT);
    EXPECT_EQ(expected_permissions, permissions_bits);
    m_shmem->unlink(); // Manually unlink unless config_shmem_u() is called
}

TEST_F(SharedMemoryTest, default_permissions_shm)
{
    std::string key_path = construct_shm_path(m_key_shm);
    default_permissions_test(m_key_shm, key_path);
}

TEST_F(SharedMemoryTest, default_permissions_file)
{
    std::string key_path = construct_shm_path(m_key_file);
    default_permissions_test(m_key_file, key_path);
}

void SharedMemoryTest::secure_permissions_test(const std::string &shm_key,
                                               const std::string &key_path)
{
    std::string key = shm_key;
    int fd = -1;
    uint32_t permissions_bits = 0;
    uint32_t expected_permissions = S_IRUSR | S_IWUSR;
    struct stat stat_struct;

    config_shmem_s(key);
    fd = open(key_path.c_str(), O_RDWR, 0);
    fstat(fd, &stat_struct);
    // Mask off type bits, save mode bits
    permissions_bits = stat_struct.st_mode & ~(S_IFMT);
    EXPECT_EQ(expected_permissions, permissions_bits);
    m_shmem->unlink(); // Manually unlink unless config_shmem_u() is called
}

TEST_F(SharedMemoryTest, secure_permissions_shm)
{
    std::string key_path = construct_shm_path(m_key_shm);
    secure_permissions_test(m_key_shm, key_path);
}

TEST_F(SharedMemoryTest, secure_permissions_file)
{
    std::string key_path = construct_shm_path(m_key_file);
    secure_permissions_test(m_key_file, key_path);
}
