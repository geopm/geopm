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

#include "geopm_time.h"
#include "geopm_endpoint.h"
#include "MockSharedMemory.hpp"
#include "Helper.hpp"
#include "Exception.hpp"
#include "EndpointImp.hpp"
#include "EndpointUser.hpp"
#include "SharedMemoryImp.hpp"

using geopm::EndpointImp;
using geopm::EndpointUserImp;
using geopm::SharedMemoryUserImp;
using geopm::geopm_endpoint_policy_shmem_s;
using geopm::geopm_endpoint_sample_shmem_s;
using geopm::Exception;
using testing::AtLeast;

class EndpointTest : public ::testing::Test
{
    protected:
        void SetUp();
        const std::string m_shm_path = "/EndpointTest_data_" + std::to_string(geteuid());
        std::unique_ptr<MockSharedMemory> m_policy_shmem;
        std::unique_ptr<MockSharedMemory> m_sample_shmem;
        bool m_cancel;
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

    m_cancel = false;
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
    EndpointImp jio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), values.size(), 0);
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
    EndpointImp gp(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, num_sample);
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
    EndpointImp mio(m_shm_path, nullptr, nullptr, values.size(), 0);
    mio.open();
    mio.write_policy(values);

    SharedMemoryUserImp smp(m_shm_path + "-policy", 1);
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) smp.pointer();

    ASSERT_LT(0u, data->count);
    std::vector<double> result(data->values, data->values + data->count);
    EXPECT_EQ(values, result);

    values[0] = 888;

    mio.write_policy(values);
    ASSERT_LT(0u, data->count);
    std::vector<double> result2(data->values, data->values + data->count);
    EXPECT_EQ(values, result2);
    mio.close();
}

TEST_F(EndpointTestIntegration, write_read_policy)
{
    std::vector<double> values = {777, 12.3456, 2.1e9};
    EndpointImp mio(m_shm_path, nullptr, nullptr, values.size(), 0);
    mio.open();
    mio.write_policy(values);
    EndpointUserImp mios(m_shm_path, nullptr, nullptr, "myagent", 0, "", "", {});

    std::vector<double> result(values.size());
    mios.read_policy(result);
    EXPECT_EQ(values, result);

    values[0] = 888;
    mio.write_policy(values);
    double age = mios.read_policy(result);
    EXPECT_EQ(values, result);
    EXPECT_LT(0.0, age);
    EXPECT_LT(age, 0.01);
    mio.close();
}

TEST_F(EndpointTestIntegration, write_read_sample)
{
    std::vector<double> values = {777, 12.3456, 2.1e9, 2.3e9};
    std::set<std::string> hosts = {"node5", "node6", "node8"};
    std::string hostlist_path = "EndpointTestIntegration_hostlist";
    EndpointImp mio(m_shm_path, nullptr, nullptr, 0, values.size());
    mio.open();
    EndpointUserImp mios(m_shm_path, nullptr, nullptr, "power_balancer",
                           values.size(), "myprofile", hostlist_path, hosts);
    EXPECT_EQ("power_balancer", mio.get_agent());
    EXPECT_EQ("myprofile", mio.get_profile_name());
    EXPECT_EQ(hosts, mio.get_hostnames());

    mios.write_sample(values);
    std::vector<double> result(values.size());
    mio.read_sample(result);
    EXPECT_EQ(values, result);

    values[0] = 888;
    mios.write_sample(values);
    mio.read_sample(result);
    EXPECT_EQ(values, result);
    mio.close();
    unlink(hostlist_path.c_str());
}

TEST_F(EndpointTest, get_agent)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    EndpointImp mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    EXPECT_EQ("monitor", mio.get_agent());
    mio.close();
}

TEST_F(EndpointTest, get_profile_name)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    EndpointImp mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();
    strncpy(data->profile_name, "my_prof", GEOPM_ENDPOINT_PROFILE_NAME_MAX);
    EXPECT_EQ("my_prof", mio.get_profile_name());
    mio.close();
}

TEST_F(EndpointTest, get_hostnames)
{
    std::set<std::string> hosts = {"node0", "node1", "node2", "node3", "node4"};
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    EndpointImp mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();
    std::string hostlist_path = "EndpointTest_hostlist";
    std::ofstream hostlist(hostlist_path);
    for (auto host : hosts) {
        hostlist << host << "\n";
    }
    hostlist.close();
    strncpy(data->hostlist_path, hostlist_path.c_str(), GEOPM_ENDPOINT_HOSTLIST_PATH_MAX);
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    EXPECT_EQ(hosts, mio.get_hostnames());
    mio.close();
    unlink(hostlist_path.c_str());
}

TEST_F(EndpointTest, cancel_stops_wait_loop)
{
    EndpointImp mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();

    auto run_thread = std::async(std::launch::async,
                                 [&mio, this] {
                                     mio.wait_for_agent_attach(m_cancel, m_timeout);
                                 });
    m_cancel = true;
    // wait for less than timeout; should exit before time limit without throwing
    auto result = run_thread.wait_for(std::chrono::seconds(m_timeout - 1));
    EXPECT_NE(result, std::future_status::timeout);
    mio.close();
}

TEST_F(EndpointTest, wait_loop_timeout_throws)
{
    EndpointImp mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();

    geopm_time_s before;
    geopm_time(&before);
    auto run_thread = std::async(std::launch::async,
                                 [&mio, this] {
                                     mio.wait_for_agent_attach(m_cancel, m_timeout);
                                 });
    // throw from our timeout should happen before longer async timeout
    std::future_status result = run_thread.wait_for(std::chrono::seconds(m_timeout + 1));
    GEOPM_EXPECT_THROW_MESSAGE(run_thread.get(),
                               GEOPM_ERROR_RUNTIME,
                               "timed out");
    EXPECT_NE(result, std::future_status::timeout);
    double elapsed = geopm_time_since(&before);
    EXPECT_NEAR(m_timeout, elapsed, 0.010);
    mio.close();
}

TEST_F(EndpointTest, wait_stops_when_agent_attaches)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    EndpointImp mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();

    auto run_thread = std::async(std::launch::async,
                                 [&mio, this] {
                                     mio.wait_for_agent_attach(m_cancel, m_timeout);
                                 });
    // simulate agent attach
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    // wait for less than timeout; should exit before time limit without throwing
    auto result = run_thread.wait_for(std::chrono::seconds(m_timeout - 1));
    EXPECT_NE(result, std::future_status::timeout);
    mio.close();
}
