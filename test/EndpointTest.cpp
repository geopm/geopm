/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <future>
#include <thread>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"

#include "geopm_field.h"
#include "geopm_time.h"
#include "geopm_endpoint.h"
#include "MockSharedMemory.hpp"
#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "EndpointImp.hpp"
#include "EndpointUser.hpp"
#include "geopm/SharedMemory.hpp"

using geopm::Endpoint;
using geopm::EndpointImp;
using geopm::EndpointUserImp;
using geopm::SharedMemory;
using geopm::geopm_endpoint_policy_shmem_s;
using geopm::geopm_endpoint_sample_shmem_s;
using geopm::Exception;
using testing::AtLeast;

class EndpointTest : public ::testing::Test
{
    protected:
        void SetUp();
        const std::string m_shm_path = "/EndpointTest_data_" + std::to_string(geteuid());
        std::shared_ptr<MockSharedMemory> m_policy_shmem;
        std::shared_ptr<MockSharedMemory> m_sample_shmem;
        int m_timeout;
};

class EndpointTestIntegration : public ::testing::Test
{
    protected:
        void TearDown();
        const std::string m_shm_path = "/EndpointTestIntegration_data_" + std::to_string(geteuid());
};

void EndpointTest::SetUp()
{
    size_t policy_shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
    m_policy_shmem = geopm::make_unique<MockSharedMemory>(policy_shmem_size);
    size_t sample_shmem_size = sizeof(struct geopm_endpoint_sample_shmem_s);
    m_sample_shmem = geopm::make_unique<MockSharedMemory>(sample_shmem_size);

    m_timeout = 2;

    EXPECT_CALL(*m_policy_shmem, get_scoped_lock()).Times(AtLeast(0));
    EXPECT_CALL(*m_policy_shmem, unlink());
    EXPECT_CALL(*m_sample_shmem, get_scoped_lock()).Times(AtLeast(0));
    EXPECT_CALL(*m_sample_shmem, unlink());
}

void EndpointTestIntegration::TearDown()
{
    unlink(("/dev/shm/" + m_shm_path + "-policy").c_str());
    unlink(("/dev/shm/" + m_shm_path + "-sample").c_str());
}

TEST_F(EndpointTest, write_shm_policy)
{
    std::vector<double> values = {777, 12.3456, 2.3e9};
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem->pointer();
    EndpointImp jio(m_shm_path, m_policy_shmem, m_sample_shmem, values.size(), 0);
    jio.open();
    jio.write_policy(values);

    std::vector<double> test = std::vector<double>(data->values, data->values + data->count);
    EXPECT_EQ(values, test);
    jio.close();
}

TEST_F(EndpointTest, parse_shm_sample)
{
    double tmp[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    int num_sample = sizeof(tmp) / sizeof(tmp[0]);
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    EndpointImp gp(m_shm_path, m_policy_shmem, m_sample_shmem, 0, num_sample);
    gp.open();
    // Build the data
    data->count = num_sample;
    memcpy(data->values, tmp, sizeof(tmp));
    geopm_time_s now;
    geopm_time(&now);
    data->timestamp = now;

    std::vector<double> result(num_sample);
    double age = gp.read_sample(result);
    std::vector<double> expected {tmp, tmp + num_sample};
    EXPECT_EQ(expected, result);
    EXPECT_LT(0.0, age);
    EXPECT_LT(age, 0.01);
    gp.close();
}

TEST_F(EndpointTestIntegration, write_shm)
{
    std::vector<double> values = {777, 12.3456, 2.1e9};
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, nullptr, nullptr, values.size(), 0);
    mio->open();
    mio->write_policy(values);

    auto smp = SharedMemory::make_unique_user(m_shm_path + "-policy", 1);
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) smp->pointer();

    ASSERT_LT(0u, data->count);
    std::vector<double> result(data->values, data->values + data->count);
    EXPECT_EQ(values, result);

    values[0] = 888;

    mio->write_policy(values);
    ASSERT_LT(0u, data->count);
    std::vector<double> result2(data->values, data->values + data->count);
    EXPECT_EQ(values, result2);
    mio->close();
}

TEST_F(EndpointTestIntegration, write_read_policy)
{
    std::vector<double> values = {777, 12.3456, 2.1e9};
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, nullptr, nullptr, values.size(), 0);
    mio->open();
    mio->write_policy(values);
    EndpointUserImp mios(m_shm_path, nullptr, nullptr, "myagent", 0, "", "", {});

    std::vector<double> result(values.size());
    mios.read_policy(result);
    EXPECT_EQ(values, result);

    values[0] = 888;
    mio->write_policy(values);
    usleep(10);
    double age = mios.read_policy(result);
    EXPECT_EQ(values, result);
    EXPECT_LT(0.0, age);
    EXPECT_LT(age, 0.01);
    mio->close();
}

TEST_F(EndpointTestIntegration, write_read_sample)
{
    std::vector<double> values = {777, 12.3456, 2.1e9, 2.3e9};
    std::set<std::string> hosts = {"node5", "node6", "node8"};
    std::string hostlist_path = "EndpointTestIntegration_hostlist";
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, nullptr, nullptr, 0, values.size());
    mio->open();
    EndpointUserImp mios(m_shm_path, nullptr, nullptr, "power_balancer",
                         values.size(), "myprofile", hostlist_path, hosts);
    EXPECT_EQ("power_balancer", mio->get_agent());
    EXPECT_EQ("myprofile", mio->get_profile_name());
    EXPECT_EQ(hosts, mio->get_hostnames());

    mios.write_sample(values);
    std::vector<double> result(values.size());
    mio->read_sample(result);
    EXPECT_EQ(values, result);

    values[0] = 888;
    mios.write_sample(values);
    mio->read_sample(result);
    EXPECT_EQ(values, result);
    mio->close();
    unlink(hostlist_path.c_str());
}

TEST_F(EndpointTest, get_agent)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    EXPECT_EQ("monitor", mio->get_agent());
    mio->close();
}

TEST_F(EndpointTest, get_profile_name)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    strncpy(data->profile_name, "my_prof", GEOPM_ENDPOINT_PROFILE_NAME_MAX);
    EXPECT_EQ("my_prof", mio->get_profile_name());
    mio->close();
}

TEST_F(EndpointTest, get_hostnames)
{
    std::set<std::string> hosts = {"node0", "node1", "node2", "node3", "node4"};
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    std::string hostlist_path = "EndpointTest_hostlist";
    std::ofstream hostlist(hostlist_path);
    for (auto host : hosts) {
        hostlist << host << "\n";
    }
    hostlist.close();
    strncpy(data->hostlist_path, hostlist_path.c_str(), GEOPM_ENDPOINT_HOSTLIST_PATH_MAX - 1);
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    EXPECT_EQ(hosts, mio->get_hostnames());
    mio->close();
    unlink(hostlist_path.c_str());
}

TEST_F(EndpointTest, stop_wait_loop)
{
    GEOPM_TEST_EXTENDED("Requires multiple threads");
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    mio->reset_wait_loop();
    auto run_thread = std::async(std::launch::async,
                                 &Endpoint::wait_for_agent_attach,
                                 mio,
                                 m_timeout);
    ASSERT_TRUE(run_thread.valid());
    mio->stop_wait_loop();
    // wait for less than timeout; should exit before time limit without throwing
    auto result = run_thread.wait_for(std::chrono::seconds(m_timeout - 1));
    EXPECT_NE(result, std::future_status::timeout);
    mio->close();
}

TEST_F(EndpointTest, attach_wait_loop_timeout_throws)
{
    GEOPM_TEST_EXTENDED("Requires multiple threads");

    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();

    geopm_time_s before;
    geopm_time(&before);

    auto run_thread = std::async(std::launch::async,
                                 &Endpoint::wait_for_agent_attach,
                                 mio,
                                 m_timeout);
    ASSERT_TRUE(run_thread.valid());
    // throw from our timeout should happen before longer async timeout
    std::future_status result = run_thread.wait_for(std::chrono::seconds(m_timeout + 1));
    GEOPM_EXPECT_THROW_MESSAGE(run_thread.get(),
                               GEOPM_ERROR_RUNTIME,
                               "timed out");
    EXPECT_NE(result, std::future_status::timeout);
    double elapsed = geopm_time_since(&before);
    EXPECT_NEAR(m_timeout, elapsed, 0.100);

    mio->close();
}

TEST_F(EndpointTest, detach_wait_loop_timeout_throws)
{
    GEOPM_TEST_EXTENDED("Requires multiple threads");

    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    // simulate agent attach
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);

    geopm_time_s before;
    geopm_time(&before);

    auto run_thread = std::async(std::launch::async,
                                 &Endpoint::wait_for_agent_detach,
                                 mio,
                                 m_timeout);
    ASSERT_TRUE(run_thread.valid());
    // throw from our timeout should happen before longer async timeout
    std::future_status result = run_thread.wait_for(std::chrono::seconds(m_timeout + 1));
    GEOPM_EXPECT_THROW_MESSAGE(run_thread.get(),
                               GEOPM_ERROR_RUNTIME,
                               "timed out");
    EXPECT_NE(result, std::future_status::timeout);
    double elapsed = geopm_time_since(&before);
    EXPECT_NEAR(m_timeout, elapsed, 0.100);

    mio->close();
}

TEST_F(EndpointTest, wait_stops_when_agent_attaches)
{
    GEOPM_TEST_EXTENDED("Requires multiple threads");

    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();

    auto run_thread = std::async(std::launch::async,
                                 &Endpoint::wait_for_agent_attach,
                                 mio,
                                 m_timeout);
    ASSERT_TRUE(run_thread.valid());
    // simulate agent attach
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    // wait for less than timeout; should exit before time limit without throwing
    auto result = run_thread.wait_for(std::chrono::seconds(m_timeout - 1));
    EXPECT_NE(result, std::future_status::timeout);
    mio->close();
}

TEST_F(EndpointTest, wait_attach_timeout_0)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();

    // if agent is not already attached, throw immediately
    GEOPM_EXPECT_THROW_MESSAGE(mio->wait_for_agent_attach(0), GEOPM_ERROR_RUNTIME, "timed out");

    // simulate agent attach
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    // wait for less than timeout; should exit before time limit without throwing

    // once agent is attached, timeout of 0 should succeed
    mio->wait_for_agent_attach(0);
    EXPECT_EQ("monitor", mio->get_agent());

    mio->close();
}

TEST_F(EndpointTest, wait_stops_when_agent_detaches)
{
    GEOPM_TEST_EXTENDED("Requires multiple threads");

    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    // simulate agent attach
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    ASSERT_EQ("monitor", mio->get_agent());

    auto run_thread = std::async(std::launch::async,
                                 &Endpoint::wait_for_agent_detach,
                                 mio,
                                 m_timeout);

    ASSERT_TRUE(run_thread.valid());
    // simulate agent detach
    strncpy(data->agent, "", GEOPM_ENDPOINT_AGENT_NAME_MAX);

    // wait for less than timeout; should exit before time limit without throwing
    auto result = run_thread.wait_for(std::chrono::seconds(m_timeout - 1));
    EXPECT_NE(result, std::future_status::timeout);
    mio->close();
}

TEST_F(EndpointTest, wait_detach_timeout_0)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    std::shared_ptr<Endpoint> mio = std::make_shared<EndpointImp>(m_shm_path, m_policy_shmem, m_sample_shmem, 0, 0);
    mio->open();
    // simulate agent attach
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);

    // if agent is still attached, throw immediately
    GEOPM_EXPECT_THROW_MESSAGE(mio->wait_for_agent_detach(0), GEOPM_ERROR_RUNTIME, "timed out");

    // simulate agent detach
    strncpy(data->agent, "", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    // once agent is detached, timeout of 0 should succeed
    mio->wait_for_agent_detach(0);
    EXPECT_EQ("", mio->get_agent());
    mio->close();
}
