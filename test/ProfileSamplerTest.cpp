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

//#include <sys/types.h>
//#include <unistd.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_env.h"
#include "geopm_sched.h"
#include "Profile.hpp"
#include "Exception.hpp"
#include "SharedMemory.hpp"
#include "MockComm.hpp"
#include "MockProfileTable.hpp"
#include "MockProfileThreadTable.hpp"
#include "MockSampleScheduler.hpp"
#include "MockControlMessage.hpp"
#include "MockSharedMemoryUser.hpp"

using geopm::Exception;
using geopm::Profile;
using geopm::IProfileThreadTable;
using geopm::ISharedMemory;
using geopm::SharedMemory;
using geopm::ISharedMemoryUser;
using geopm::SharedMemoryUser;
using geopm::IProfileTable;
using geopm::ISampleScheduler;
using geopm::IControlMessage;

struct free_delete
{
    void operator()(void* x)
    {
        free(x);
    }
};

class ProfileTestSharedMemoryUser : public MockSharedMemoryUser
{
    protected:
        std::unique_ptr<void, free_delete> m_buffer;
    public:
        ProfileTestSharedMemoryUser()
        {
        }

        ProfileTestSharedMemoryUser(size_t size)
        {
            m_buffer = std::unique_ptr<void, free_delete>(malloc(size));
            EXPECT_CALL(*this, size())
                .WillRepeatedly(testing::Return(size));
            EXPECT_CALL(*this, pointer())
                .WillRepeatedly(testing::Return(m_buffer.get()));
            EXPECT_CALL(*this, unlink())
                .WillRepeatedly(testing::Return());
        }
};

class ProfileTestControlMessage : public MockControlMessage
{
    public:
        ProfileTestControlMessage()
        {
            EXPECT_CALL(*this, step())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, wait())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, cpu_rank(testing::_, testing::_))
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, cpu_rank(testing::_))
                .WillRepeatedly(testing::Return(0));
            EXPECT_CALL(*this, loop_begin())
                .WillRepeatedly(testing::Return());
        }
};

class ProfileTestSampleScheduler : public MockSampleScheduler
{
    public:
        ProfileTestSampleScheduler()
        {
            EXPECT_CALL(*this, clear())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, do_sample())
                .WillRepeatedly(testing::Return(true));
        }
};

class ProfileTestProfileTable : public MockProfileTable
{
    public:
        ProfileTestProfileTable(std::function<uint64_t (const std::string &)> key_lambda, std::function<void (uint64_t key, const struct geopm_prof_message_s &value)> insert_lambda)
        {
            EXPECT_CALL(*this, key(testing::_))
                .WillRepeatedly(testing::Invoke(key_lambda));
            EXPECT_CALL(*this, insert(testing::_, testing::_))
                .WillRepeatedly(testing::Invoke(insert_lambda));
        }
};

class ProfileTestProfileThreadTable : public MockProfileThreadTable
{
    public:
        ProfileTestProfileThreadTable()
        {
        }
};

class ProfileTestComm : public MockComm
{
    public:
        // COMM_WORLD
        ProfileTestComm(int world_rank, std::shared_ptr<MockComm> shm_comm)
        {
            EXPECT_CALL(*this, rank())
                .WillRepeatedly(testing::Return(world_rank));
            EXPECT_CALL(*this, split("prof", IComm::M_COMM_SPLIT_TYPE_SHARED))
                .WillOnce(testing::Return(shm_comm));
            EXPECT_CALL(*this, barrier())
                .WillRepeatedly(testing::Return());
        }

        ProfileTestComm(int shm_rank, int shm_size)
        {
            //rank, num_rank, barrier, test,
            EXPECT_CALL(*this, rank())
                .WillRepeatedly(testing::Return(shm_rank));
            EXPECT_CALL(*this, num_rank())
                .WillRepeatedly(testing::Return(shm_size));
            EXPECT_CALL(*this, barrier())
                .WillRepeatedly(testing::Return());
            EXPECT_CALL(*this, test(testing::_))
                .WillRepeatedly(testing::Return(true));
        }
};

class ProfileSamplerTest : public :: testing :: Test
{
    public:
        ProfileSamplerTest();
        ~ProfileSamplerTest();
};

ProfileSamplerTest::ProfileSamplerTest()
{
}

ProfileSamplerTest::~ProfileSamplerTest()
{
}

TEST_F(ProfileSamplerTest, hello)
{
}
