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
#include <pthread.h>
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
using geopm::FileEndpoint;
using geopm::FileEndpointUser;
using geopm::SharedMemoryImp;
using geopm::geopm_endpoint_shmem_s;
using geopm::Exception;

using json11::Json;

class FileEndpointTest : public ::testing::Test
{
    protected:
        void SetUp();
        const std::string m_json_file_path = "FileEndpointTest_data";
        std::string m_valid_json;
};

class ShmemEndpointTest : public ::testing::Test
{
    protected:
        const std::string m_shm_path = "/ShmemEndpointTest_data_" + std::to_string(geteuid());
};

class ShmemEndpointTestIntegration : public ShmemEndpointTest
{

};

void FileEndpointTest::SetUp()
{
    std::string tab = std::string(4, ' ');
    std::ostringstream valid_json;
    valid_json << "{" << std::endl
                 << tab << "\"POWER_CONSUMED\" : 777," << std::endl
                 << tab << "\"RUNTIME\" : 12.3456," << std::endl
                 << tab << "\"GHZ\" : 2.3e9" << std::endl
                 << "}" << std::endl;
    m_valid_json = valid_json.str();
}

TEST_F(FileEndpointTest, write_json_file)
{
    std::vector<std::string> signal_names = {"POWER_CONSUMED", "RUNTIME", "GHZ"};
    FileEndpoint jio(m_json_file_path, signal_names);

    std::vector<double> values = {2.3e9, 12.3456, 777};
    jio.adjust(values);
    jio.write_batch();

    FileEndpointUser jios(m_json_file_path, signal_names);

    std::vector<double> result = jios.sample();
    EXPECT_EQ(values, result);

    std::remove(m_json_file_path.c_str());
}

TEST_F(ShmemEndpointTest, write_shm)
{
    size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
    std::unique_ptr<MockSharedMemory> shmem(new MockSharedMemory(shmem_size));
    struct geopm_endpoint_shmem_s *data = (struct geopm_endpoint_shmem_s *) shmem->pointer();

    std::vector<std::string> signal_names = {"POWER_CONSUMED", "RUNTIME", "GHZ"};
    ShmemEndpoint jio(m_shm_path, std::move(shmem), signal_names);

    std::vector<double> values = {777, 12.3456, 2.3e9};
    jio.adjust(values);
    jio.write_batch();

    std::vector<double> test = std::vector<double>(data->values, data->values + signal_names.size());

    EXPECT_EQ(values, test);
}

TEST_F(FileEndpointTest, negative_write_json_file)
{
    std::string path ("EndpointTest_empty");
    std::ofstream empty_file(path, std::ofstream::out);
    empty_file.close();
    chmod(path.c_str(), 0);

    const std::vector<std::string> signal_names = {"FAKE_SIGNAL"};
    FileEndpoint jio (path, signal_names);

    GEOPM_EXPECT_THROW_MESSAGE(jio.write_batch(),
                               EACCES, "file \"" + path + "\" could not be opened");
    std::remove(path.c_str());
}

TEST_F(ShmemEndpointTestIntegration, write_shm)
{
    std::vector<std::string> signal_names = {"POWER_CONSUMED", "RUNTIME", "GHZ1", "GHZ2", "GHZ3", "GHZ4", "GHZ5", "GHZ6",
                                             "GHZ7", "GHZ8"};
    ShmemEndpoint mio(m_shm_path, nullptr, signal_names);

    std::vector<double> values = {777, 12.3456, 2.1e9, 2.3e9, 2.5e9,
                                  2.6e9, 2.7e9, 2.8e9, 2.4e9, 2.2e9};
    mio.adjust(values);
    mio.write_batch();

    ShmemEndpointUser mios(m_shm_path, nullptr, signal_names);

    std::vector<double> result = mios.sample();
    EXPECT_EQ(values, result);
}


/*************************************************************************************************/

class FileEndpointUserTest: public ::testing::Test
{
    protected:
        void SetUp();
        void TearDown();
        const std::string m_json_file_path = "ShmemEndpointUserTest_data";
        const std::string m_json_file_path_bad = "ShmemEndpointUserTest_data_bad";

        std::string m_valid_json;
        std::string m_valid_json_bad_type;
};

class ShmemEndpointUserTest: public ::testing::Test
{
    protected:
        const std::string m_shm_path = "/ShmemEndpointUserTest_data_" + std::to_string(geteuid());
};

class ShmemEndpointUserTestIntegration : public ShmemEndpointUserTest
{

};

void FileEndpointUserTest::SetUp()
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

void FileEndpointUserTest::TearDown()
{
    std::remove(m_json_file_path.c_str());
    std::remove(m_json_file_path_bad.c_str());
}

TEST_F(FileEndpointUserTest, parse_json_file)
{
    std::vector<std::string> signal_names = {"POWER_MAX", "FREQUENCY_MAX", "FREQUENCY_MIN", "PI",
                                             "DEFAULT1", "DEFAULT2", "DEFAULT3"};
    FileEndpointUser gp(m_json_file_path, signal_names);

    std::vector<double> result = gp.sample();
    ASSERT_EQ(7u, result.size());
    EXPECT_EQ(400, result[0]);
    EXPECT_EQ(2.3e9, result[1]);
    EXPECT_EQ(1.2e9, result[2]);
    EXPECT_EQ(3.14159265, result[3]);
    EXPECT_TRUE(std::isnan(result[4]));
    EXPECT_TRUE(std::isnan(result[5]));
    EXPECT_TRUE(std::isnan(result[6]));
}

TEST_F(FileEndpointUserTest, negative_parse_json_file)
{
    const std::vector<std::string> signal_names = {"FAKE_SIGNAL"};
    GEOPM_EXPECT_THROW_MESSAGE(new FileEndpointUser(m_json_file_path_bad, signal_names),
                               GEOPM_ERROR_FILE_PARSE, "unsupported type or malformed json config file");

    // Don't parse if Agent doesn't require any policies
    const std::vector<std::string> signal_names_empty;
    FileEndpointUser("", signal_names_empty);
}

TEST_F(ShmemEndpointUserTest, parse_shm)
{
    size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
    std::unique_ptr<MockSharedMemoryUser> shmem(new MockSharedMemoryUser(shmem_size));
    struct geopm_endpoint_shmem_s *data = (struct geopm_endpoint_shmem_s *) shmem->pointer();

    // Build the data
    data->is_updated = true;
    ShmemEndpoint::setup_mutex(data->lock);
    double tmp[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    data->count = sizeof(tmp) / sizeof(tmp[0]);
    memcpy(data->values, tmp, sizeof(tmp));

    std::vector<std::string> signal_names = {"ONE", "TWO", "THREE", "FOUR", "FIVE"};
    ShmemEndpointUser gp("/FAKE_PATH", std::move(shmem), signal_names);

    EXPECT_FALSE(gp.is_update_available());
    std::vector<double> result = gp.sample();
    std::vector<double> expected {tmp, tmp + signal_names.size()};
    EXPECT_EQ(expected, result);
}

TEST_F(ShmemEndpointUserTest, negative_parse_shm)
{
    size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
    std::unique_ptr<MockSharedMemoryUser> shmem(new MockSharedMemoryUser(shmem_size));
    struct geopm_endpoint_shmem_s *data = (struct geopm_endpoint_shmem_s *) shmem->pointer();

    // Build the data
    data->is_updated = false; // This will force the parsing logic to throw since the structure is "not updated".
    ShmemEndpoint::setup_mutex(data->lock);

    double tmp[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    data->count = sizeof(tmp) / sizeof(tmp[0]);
    memcpy(data->values, tmp, sizeof(tmp));

    std::vector<std::string> signal_names = {"ONE", "TWO", "THREE", "FOUR", "FIVE"};
    GEOPM_EXPECT_THROW_MESSAGE(new ShmemEndpointUser("/FAKE_PATH", std::move(shmem), signal_names),
                               GEOPM_ERROR_INVALID, "reread of shm region requested before update");
}

TEST_F(ShmemEndpointUserTest, negative_shm_setup_mutex)
{
    // This test requires PTHREAD_MUTEX_ERRORCHECK
    size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
    std::unique_ptr<MockSharedMemoryUser> shmem(new MockSharedMemoryUser(shmem_size));
    struct geopm_endpoint_shmem_s *data = (struct geopm_endpoint_shmem_s *) shmem->pointer();
    *data = {};

    // Build the data
    data->is_updated = true;
    ShmemEndpoint::setup_mutex(data->lock);
    (void) pthread_mutex_lock(&data->lock); // Force pthread_mutex_lock to puke by trying to lock a locked mutex.

    GEOPM_EXPECT_THROW_MESSAGE(new ShmemEndpointUser("/FAKE_PATH", std::move(shmem), {""}),
                               EDEADLK, "Resource deadlock avoided");
}

TEST_F(FileEndpointUserTest, negative_bad_files)
{
    std::string path ("FileEndpointUserTest_empty");
    std::ofstream empty_file(path, std::ofstream::out);
    empty_file.close();
    const std::vector<std::string> signal_names = {"FAKE_SIGNAL"};
    GEOPM_EXPECT_THROW_MESSAGE(new FileEndpointUser(path, signal_names),
                               GEOPM_ERROR_INVALID, "input file invalid");
    chmod(path.c_str(), 0);
    GEOPM_EXPECT_THROW_MESSAGE(new FileEndpointUser(path, signal_names),
                               EACCES, "file \"" + path + "\" could not be opened");
    std::remove(path.c_str());
}

TEST_F(ShmemEndpointUserTestIntegration, parse_shm)
{
    std::string full_path("/dev/shm" + m_shm_path);
    std::remove(full_path.c_str());

    size_t shmem_size = sizeof(struct geopm_endpoint_shmem_s);
    SharedMemoryImp sm(m_shm_path, shmem_size);
    struct geopm_endpoint_shmem_s *data = (struct geopm_endpoint_shmem_s *) sm.pointer();

    // Build the data
    data->is_updated = true;
    ShmemEndpoint::setup_mutex(data->lock);
    double tmp[] = { 1.1, 2.2, 3.3, 4.4, 5.5 };
    data->count = sizeof(tmp) / sizeof(tmp[0]);
    memcpy(data->values, tmp, sizeof(tmp));

    std::vector<std::string> signal_names = {"ONE", "TWO", "THREE", "FOUR", "FIVE"};
    ShmemEndpointUser gp(m_shm_path, nullptr, signal_names);

    EXPECT_FALSE(gp.is_update_available());
    std::vector<double> result = gp.sample();
    std::vector<double> expected {tmp, tmp + signal_names.size()};
    EXPECT_EQ(expected, result);

    tmp[0] = 1.5;
    pthread_mutex_lock(&data->lock);
    memcpy(data->values, tmp, sizeof(tmp));
    data->is_updated = true;
    pthread_mutex_unlock(&data->lock);

    gp.read_batch();

    EXPECT_FALSE(gp.is_update_available());
    result = gp.sample();
    expected = {tmp, tmp + signal_names.size()};
    EXPECT_EQ(expected, result);

    std::remove(full_path.c_str());
}
