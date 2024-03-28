/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"

#include "geopm_endpoint.h"
#include "EndpointUser.hpp"
#include "EndpointImp.hpp"
#include "geopm/SharedMemory.hpp"
#include "geopm/Helper.hpp"
#include "MockSharedMemory.hpp"

using geopm::EndpointUserImp;
using geopm::SharedMemory;
using geopm::geopm_endpoint_policy_shmem_s;
using geopm::geopm_endpoint_sample_shmem_s;
using testing::AtLeast;

class EndpointUserTest : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        const std::string m_shm_path = "/EndpointUserTest_data_" + std::to_string(geteuid());
        std::string m_hostlist_file = "EndpointUserTest_hosts";
        std::unique_ptr<MockSharedMemory> m_policy_shmem_user;
        std::unique_ptr<MockSharedMemory> m_sample_shmem_user;
};

class EndpointUserTestIntegration : public ::testing::Test
{
    protected:
        void TearDown();
        const std::string m_shm_path = "/EndpointUserTestIntegration_data_" + std::to_string(geteuid());
};

void EndpointUserTest::SetUp()
{
    size_t policy_shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
    m_policy_shmem_user = geopm::make_unique<MockSharedMemory>(policy_shmem_size);
    size_t sample_shmem_size = sizeof(struct geopm_endpoint_sample_shmem_s);
    m_sample_shmem_user = geopm::make_unique<MockSharedMemory>(sample_shmem_size);

    EXPECT_CALL(*m_policy_shmem_user, get_scoped_lock()).Times(AtLeast(0));
    EXPECT_CALL(*m_sample_shmem_user, get_scoped_lock()).Times(AtLeast(0));
}

void EndpointUserTest::TearDown()
{
    unlink(m_hostlist_file.c_str());
}

void EndpointUserTestIntegration::TearDown()
{
    int uid = getuid();
    std::stringstream policy_path_user;
    std::stringstream policy_path_shm;
    std::stringstream sample_path_user;
    std::stringstream sample_path_shm;

    policy_path_user << "/run/user/" << uid << "/" << m_shm_path << "-policy";
    policy_path_shm << "/dev/shm/" << m_shm_path << "-policy";
    sample_path_user << "/run/user/" << uid << "/" << m_shm_path << "-sample";
    sample_path_shm << "/dev/shm/" << m_shm_path << "-sample";
    unlink(policy_path_user.str().c_str());
    unlink(policy_path_shm.str().c_str());
    unlink(sample_path_user.str().c_str());
    unlink(sample_path_shm.str().c_str());
}

TEST_F(EndpointUserTest, attach)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem_user->pointer();
    std::set<std::string> hosts {"node1", "node2", "node4"};
    EndpointUserImp gp("/FAKE_PATH", std::move(m_policy_shmem_user),
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

TEST_F(EndpointUserTest, parse_shm_policy)
{
    double tmp[] = { 1.1, 2.2, 3.3 };
    int num_policy = sizeof(tmp) / sizeof(tmp[0]);
    // Build the data
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem_user->pointer();
    data->count = num_policy;
    memcpy(data->values, tmp, sizeof(tmp));

    EndpointUserImp gp("/FAKE_PATH", std::move(m_policy_shmem_user),
                         std::move(m_sample_shmem_user), "myagent", 0,
                         "myprofile", m_hostlist_file, {});

    std::vector<double> result(num_policy);
    gp.read_policy(result);
    std::vector<double> expected {tmp, tmp + num_policy};
    EXPECT_EQ(expected, result);
}

TEST_F(EndpointUserTest, write_shm_sample)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem_user->pointer();
    std::vector<double> values = {777, 12.3456, 2.3e9};
    EndpointUserImp jio("/FAKE_PATH", std::move(m_policy_shmem_user), std::move(m_sample_shmem_user),
                          "myagent", values.size(), "myprofile", m_hostlist_file, {});
    jio.write_sample(values);

    std::vector<double> test = std::vector<double>(data->values, data->values + data->count);
    EXPECT_EQ(values, test);
}

TEST_F(EndpointUserTest, agent_name_too_long)
{
    std::string too_long(GEOPM_ENDPOINT_AGENT_NAME_MAX, 'X');
    std::set<std::string> hosts;

    GEOPM_EXPECT_THROW_MESSAGE(
        geopm::make_unique<EndpointUserImp>("/FAKE_PATH",
                                            std::move(m_policy_shmem_user),
                                            std::move(m_sample_shmem_user),
                                            too_long,
                                            0,
                                            "myprofile",
                                            m_hostlist_file,
                                            hosts),
        GEOPM_ERROR_INVALID, "Agent name is too long");
}

TEST_F(EndpointUserTest, profile_name_too_long)
{
    std::string too_long(GEOPM_ENDPOINT_PROFILE_NAME_MAX, 'X');
    std::set<std::string> hosts;
    GEOPM_EXPECT_THROW_MESSAGE(
        geopm::make_unique<EndpointUserImp>("/FAKE_PATH",
                                            std::move(m_policy_shmem_user),
                                            std::move(m_sample_shmem_user),
                                            "myagent",
                                            0,
                                            too_long,
                                            m_hostlist_file,
                                            hosts),
        GEOPM_ERROR_INVALID, "Profile name is too long");
}

TEST_F(EndpointUserTestIntegration, parse_shm)
{
    std::string full_path("/dev/shm" + m_shm_path);

    size_t shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
    auto smp = SharedMemory::make_unique_owner(m_shm_path + "-policy", shmem_size);
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) smp->pointer();
    auto sms = SharedMemory::make_unique_owner(m_shm_path + "-sample", sizeof(struct geopm_endpoint_sample_shmem_s));

    double tmp[] = { 1.1, 2.2, 3.3 };
    int num_policy = sizeof(tmp) / sizeof(tmp[0]);
    geopm_time_s now;
    geopm_time(&now);
    data->count = num_policy;
    // Build the data
    memcpy(data->values, tmp, sizeof(tmp));
    geopm_time(&data->timestamp);

    EndpointUserImp gp(m_shm_path, nullptr, nullptr, "myagent", 0, "myprofile", "", {});

    std::vector<double> result(num_policy);
    double age = gp.read_policy(result);
    EXPECT_LT(0.0, age);
    EXPECT_LT(age, 0.01);
    std::vector<double> expected {tmp, tmp + num_policy};
    EXPECT_EQ(expected, result);

    tmp[0] = 1.5;
    memcpy(data->values, tmp, sizeof(tmp));

    gp.read_policy(result);
    expected = {tmp, tmp + num_policy};
    EXPECT_EQ(expected, result);
}
