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

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"

#include "MockSharedMemoryUser.hpp"


class ShmemEndpointUserTest : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        const std::string m_shm_path = "/ShmemEndpointUserTest_data_" + std::to_string(geteuid());
        std::string m_hostlist_file = "ShmemEndpointUserTest_hosts";
        std::unique_ptr<MockSharedMemoryUser> m_policy_shmem_user;
        std::unique_ptr<MockSharedMemoryUser> m_sample_shmem_user;
};

class ShmemEndpointUserTestIntegration : public ::testing::Test
{
    protected:
        void TearDown();
        const std::string m_shm_path = "/ShmemEndpointUserTestIntegration_data_" + std::to_string(geteuid());
};

void ShmemEndpointUserTest::SetUp()
{
    size_t policy_shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
    m_policy_shmem_user = geopm::make_unique<MockSharedMemoryUser>(policy_shmem_size);
    size_t sample_shmem_size = sizeof(struct geopm_endpoint_sample_shmem_s);
    m_sample_shmem_user = geopm::make_unique<MockSharedMemoryUser>(sample_shmem_size);

    EXPECT_CALL(*m_policy_shmem_user, get_scoped_lock()).Times(AtLeast(0));
    EXPECT_CALL(*m_sample_shmem_user, get_scoped_lock()).Times(AtLeast(0));
}

void ShmemEndpointUserTest::TearDown()
{
    unlink(m_hostlist_file.c_str());
}

void ShmemEndpointUserTestIntegration::TearDown()
{
    unlink(("/dev/shm/" + m_shm_path + "-policy").c_str());
    unlink(("/dev/shm/" + m_shm_path + "-sample").c_str());
}

TEST_F(ShmemEndpointUserTest, attach)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem_user->pointer();
    std::set<std::string> hosts {"node1", "node2", "node4"};
    ShmemEndpointUser gp("/FAKE_PATH", std::move(m_policy_shmem_user),
                         std::move(m_sample_shmem_user), "myagent", 0,
                         "myprofile", m_hostlist_file, hosts);
    EXPECT_STREQ("myagent", data->agent);
    EXPECT_STREQ("myprofile", data->profile_name);
    EXPECT_STREQ(m_hostlist_file.c_str(), data->hostlist_path);
    std::ifstream hostlist_file(m_hostlist_file);
    std::set<std::string> hostlist;
    std::string host;
    while (hostlist_file >> host) {
        hostlist.insert(host);
    }
    EXPECT_EQ(hostlist, hosts);
}

TEST_F(ShmemEndpointUserTest, parse_shm_policy)
{
    double tmp[] = { 1.1, 2.2, 3.3 };
    int num_policy = sizeof(tmp) / sizeof(tmp[0]);
    // Build the data
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem_user->pointer();
    data->count = num_policy;
    memcpy(data->values, tmp, sizeof(tmp));

    ShmemEndpointUser gp("/FAKE_PATH", std::move(m_policy_shmem_user),
                         std::move(m_sample_shmem_user), "myagent", 0,
                         "myprofile", m_hostlist_file, {});

    std::vector<double> result(num_policy);
    gp.read_policy(result);
    std::vector<double> expected {tmp, tmp + num_policy};
    EXPECT_EQ(expected, result);
}

TEST_F(ShmemEndpointUserTest, write_shm_sample)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem_user->pointer();
    std::vector<double> values = {777, 12.3456, 2.3e9};
    ShmemEndpointUser jio("/FAKE_PATH", std::move(m_policy_shmem_user), std::move(m_sample_shmem_user),
                          "myagent", values.size(), "myprofile", m_hostlist_file, {});
    jio.write_sample(values);

    std::vector<double> test = std::vector<double>(data->values, data->values + data->count);
    EXPECT_EQ(values, test);
}

TEST_F(ShmemEndpointUserTestIntegration, parse_shm)
{
    std::string full_path("/dev/shm" + m_shm_path);

    size_t shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
    SharedMemoryImp smp(m_shm_path + "-policy", shmem_size);
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) smp.pointer();
    SharedMemoryImp sms(m_shm_path + "-sample", sizeof(struct geopm_endpoint_sample_shmem_s));

    double tmp[] = { 1.1, 2.2, 3.3 };
    int num_policy = sizeof(tmp) / sizeof(tmp[0]);
    data->count = num_policy;
    // Build the data
    memcpy(data->values, tmp, sizeof(tmp));

    ShmemEndpointUser gp(m_shm_path, nullptr, nullptr, "myagent", 0, "myprofile", "", {});

    std::vector<double> result(num_policy);
    gp.read_policy(result);
    std::vector<double> expected {tmp, tmp + num_policy};
    EXPECT_EQ(expected, result);

    tmp[0] = 1.5;
    memcpy(data->values, tmp, sizeof(tmp));

    gp.read_policy(result);
    expected = {tmp, tmp + num_policy};
    EXPECT_EQ(expected, result);
}
