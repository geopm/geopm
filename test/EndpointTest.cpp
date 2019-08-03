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

#include "contrib/json11/json11.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm_test.hpp"

#include "MockSharedMemory.hpp"
#include "MockSharedMemoryUser.hpp"

#include "Helper.hpp"
#include "Exception.hpp"
#include "Endpoint.hpp"
#include "SharedMemoryImp.hpp"

using geopm::ShmemEndpoint;
using geopm::ShmemEndpointUser;
using geopm::FilePolicy;
using geopm::SharedMemoryImp;
using geopm::SharedMemoryUserImp;
using geopm::geopm_endpoint_policy_shmem_s;
using geopm::geopm_endpoint_sample_shmem_s;
using geopm::Exception;
using testing::AtLeast;
using json11::Json;

class ShmemEndpointTest : public ::testing::Test
{
    protected:
        void SetUp();
        const std::string m_shm_path = "/ShmemEndpointTest_data_" + std::to_string(geteuid());
        std::unique_ptr<MockSharedMemory> m_policy_shmem;
        std::unique_ptr<MockSharedMemory> m_sample_shmem;
};

class ShmemEndpointTestIntegration : public ::testing::Test
{
    protected:
        void TearDown();
        const std::string m_shm_path = "/ShmemEndpointTestIntegration_data_" + std::to_string(geteuid());
};

void ShmemEndpointTest::SetUp()
{
    size_t policy_shmem_size = sizeof(struct geopm_endpoint_policy_shmem_s);
    m_policy_shmem = geopm::make_unique<MockSharedMemory>(policy_shmem_size);
    size_t sample_shmem_size = sizeof(struct geopm_endpoint_sample_shmem_s);
    m_sample_shmem = geopm::make_unique<MockSharedMemory>(sample_shmem_size);

    EXPECT_CALL(*m_policy_shmem, get_scoped_lock()).Times(AtLeast(0));
    EXPECT_CALL(*m_policy_shmem, unlink());
    EXPECT_CALL(*m_sample_shmem, get_scoped_lock()).Times(AtLeast(0));
    EXPECT_CALL(*m_sample_shmem, unlink());
}

void ShmemEndpointTestIntegration::TearDown()
{
    unlink(("/dev/shm/" + m_shm_path + "-policy").c_str());
    unlink(("/dev/shm/" + m_shm_path + "-sample").c_str());
}

TEST_F(ShmemEndpointTest, write_shm_policy)
{
    std::vector<double> values = {777, 12.3456, 2.3e9};
    struct geopm_endpoint_policy_shmem_s *data = (struct geopm_endpoint_policy_shmem_s *) m_policy_shmem->pointer();
    ShmemEndpoint jio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), values.size(), 0);
    jio.open();
    jio.write_policy(values);

    std::vector<double> test = std::vector<double>(data->values, data->values + data->count);
    EXPECT_EQ(values, test);
    jio.close();
}

TEST_F(ShmemEndpointTest, parse_shm_sample)
{
    double tmp[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    int num_sample = sizeof(tmp) / sizeof(tmp[0]);
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    ShmemEndpoint gp(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, num_sample);
    gp.open();
    // Build the data
    data->count = num_sample;
    memcpy(data->values, tmp, sizeof(tmp));
    geopm_time_s now;
    geopm_time(&now);
    data->timestamp = now;

    std::vector<double> result(num_sample);
    geopm_time_s ts = gp.read_sample(result);
    std::vector<double> expected {tmp, tmp + num_sample};
    EXPECT_EQ(expected, result);
    EXPECT_DOUBLE_EQ(0.0, geopm_time_diff(&now, &ts));
    gp.close();
}

TEST_F(ShmemEndpointTestIntegration, write_shm)
{
    std::vector<double> values = {777, 12.3456, 2.1e9};
    ShmemEndpoint mio(m_shm_path, nullptr, nullptr, values.size(), 0);
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

TEST_F(ShmemEndpointTestIntegration, write_read_policy)
{
    std::vector<double> values = {777, 12.3456, 2.1e9};
    ShmemEndpoint mio(m_shm_path, nullptr, nullptr, values.size(), 0);
    mio.open();
    mio.write_policy(values);
    ShmemEndpointUser mios(m_shm_path, nullptr, nullptr, "myagent", 0, "", "", {});

    std::vector<double> result(values.size());
    mios.read_policy(result);
    EXPECT_EQ(values, result);

    values[0] = 888;
    mio.write_policy(values);
    mios.read_policy(result);
    EXPECT_EQ(values, result);
    mio.close();
}

TEST_F(ShmemEndpointTestIntegration, write_read_sample)
{
    std::vector<double> values = {777, 12.3456, 2.1e9, 2.3e9};
    std::vector<std::string> hosts = {"node5", "node6", "node8"};
    std::string hostlist_path = "ShmemEndpointTestIntegration_hostlist";
    ShmemEndpoint mio(m_shm_path, nullptr, nullptr, 0, values.size());
    mio.open();
    ShmemEndpointUser mios(m_shm_path, nullptr, nullptr, "power_balancer",
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

TEST_F(ShmemEndpointTest, get_agent)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    ShmemEndpoint mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    EXPECT_EQ("monitor", mio.get_agent());
    mio.close();
}

TEST_F(ShmemEndpointTest, get_profile_name)
{
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    ShmemEndpoint mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();
    strncpy(data->profile_name, "my_prof", GEOPM_ENDPOINT_PROFILE_NAME_MAX);
    EXPECT_EQ("my_prof", mio.get_profile_name());
    mio.close();
}

TEST_F(ShmemEndpointTest, get_hostnames)
{
    std::vector<std::string> hosts = {"node0", "node1", "node2", "node3", "node4"};
    struct geopm_endpoint_sample_shmem_s *data = (struct geopm_endpoint_sample_shmem_s *) m_sample_shmem->pointer();
    ShmemEndpoint mio(m_shm_path, std::move(m_policy_shmem), std::move(m_sample_shmem), 0, 0);
    mio.open();
    std::string hostfile_path = "ShmemEndpointTest_hostlist";
    std::ofstream hostfile(hostfile_path);
    for (auto host : hosts) {
        hostfile << host << "\n";
    }
    hostfile.close();
    strncpy(data->hostlist_path, hostfile_path.c_str(), GEOPM_ENDPOINT_HOSTFILE_PATH_MAX);
    strncpy(data->agent, "monitor", GEOPM_ENDPOINT_AGENT_NAME_MAX);
    EXPECT_EQ(hosts, mio.get_hostnames());
    mio.close();
    unlink(hostfile_path.c_str());
}

/*************************************************************************************************/

class FilePolicyTest : public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        const std::string m_json_file_path = "FilePolicyTest_data";
        const std::string m_json_file_path_bad = "FilePolicyTest_data_bad";

        std::string m_valid_json;
        std::string m_valid_json_bad_type;
};

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

void FilePolicyTest::SetUp()
{
    std::string tab = std::string(4, ' ');
    std::ostringstream valid_json;
    valid_json << "{" << std::endl
               << tab << "\"POWER_MAX\" : 400," << std::endl
               << tab << "\"FREQUENCY_MAX\" : 2300000000," << std::endl
               << tab << "\"FREQUENCY_MIN\" : 1200000000," << std::endl
               << tab << "\"PI\" : 3.14159265," << std::endl
               << tab << "\"GHZ\" : 2.3e9," << std::endl
               << tab << "\"DEFAULT1\" : \"NAN\"," << std::endl
               << tab << "\"DEFAULT2\" : \"nan\"," << std::endl
               << tab << "\"DEFAULT3\" : \"NaN\"" << std::endl
               << "}" << std::endl;
    m_valid_json = valid_json.str();

    std::ostringstream bad_json;
    bad_json << "{" << std::endl
             << tab << "\"POWER_MAX\" : 400," << std::endl
             << tab << "\"FREQUENCY_MAX\" : 2300000000," << std::endl
             << tab << "\"FREQUENCY_MIN\" : 1200000000," << std::endl
             << tab << "\"ARBITRARY_SIGNAL\" : \"WUBBA LUBBA DUB DUB\"," << std::endl // Strings are not handled.
             << tab << "\"PI\" : 3.14159265," << std::endl
             << tab << "\"GHZ\" : 2.3e9" << std::endl
             << "}" << std::endl;
    m_valid_json_bad_type = bad_json.str();

    std::ofstream json_stream(m_json_file_path);
    std::ofstream json_stream_bad(m_json_file_path_bad);

    json_stream << m_valid_json;
    json_stream.close();

    json_stream_bad << m_valid_json_bad_type;
    json_stream_bad.close();
}

void FilePolicyTest::TearDown()
{
    unlink(m_json_file_path.c_str());
    unlink(m_json_file_path_bad.c_str());
}

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
    std::vector<std::string> hosts {"node1", "node2", "node4"};
    ShmemEndpointUser gp("/FAKE_PATH", std::move(m_policy_shmem_user),
                         std::move(m_sample_shmem_user), "myagent", 0,
                         "myprofile", m_hostlist_file, hosts);
    EXPECT_STREQ("myagent", data->agent);
    EXPECT_STREQ("myprofile", data->profile_name);
    EXPECT_STREQ(m_hostlist_file.c_str(), data->hostlist_path);
    // todo: check list of hosts
    std::ifstream hostfile(m_hostlist_file);
    std::vector<std::string> hostlist;
    std::string host;
    while (hostfile >> host) {
        hostlist.push_back(host);
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

TEST_F(FilePolicyTest, parse_json_file)
{
    std::vector<std::string> signal_names = {"POWER_MAX", "FREQUENCY_MAX", "FREQUENCY_MIN", "PI",
                                             "DEFAULT1", "DEFAULT2", "DEFAULT3"};
    std::vector<double> result = FilePolicy::read_policy(m_json_file_path, signal_names);
    ASSERT_EQ(7u, result.size());
    EXPECT_EQ(400, result[0]);
    EXPECT_EQ(2.3e9, result[1]);
    EXPECT_EQ(1.2e9, result[2]);
    EXPECT_EQ(3.14159265, result[3]);
    EXPECT_TRUE(std::isnan(result[4]));
    EXPECT_TRUE(std::isnan(result[5]));
    EXPECT_TRUE(std::isnan(result[6]));
}

TEST_F(FilePolicyTest, negative_parse_json_file)
{
    const std::vector<std::string> signal_names = {"FAKE_SIGNAL"};
    std::vector<double> policy {NAN};
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy::read_policy(m_json_file_path_bad, signal_names),
                               GEOPM_ERROR_FILE_PARSE, "unsupported type or malformed json config file");

    // Don't parse if Agent doesn't require any policies
    const std::vector<std::string> signal_names_empty;
    auto empty_result = FilePolicy::read_policy("", signal_names_empty);
    EXPECT_EQ(0u, empty_result.size());
}

TEST_F(FilePolicyTest, negative_bad_files)
{
    std::string path ("FilePolicyTest_empty");
    std::ofstream empty_file(path, std::ofstream::out);
    empty_file.close();
    const std::vector<std::string> signal_names = {"FAKE_SIGNAL"};
    std::vector<double> policy {NAN};
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy::read_policy(path, signal_names),
                               GEOPM_ERROR_INVALID, "input file invalid");
    chmod(path.c_str(), 0);
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy::read_policy(path, signal_names),
                               EACCES, "file \"" + path + "\" could not be opened");
    unlink(path.c_str());
}
