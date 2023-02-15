/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string>
#include <fstream>
#include <sstream>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FilePolicy.hpp"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

using geopm::FilePolicy;

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

void FilePolicyTest::SetUp()
{
    std::string tab = std::string(4, ' ');
    std::ostringstream valid_json;
    valid_json << "{" << std::endl
               << tab << "\"POWER_MAX\" : 400," << std::endl
               << tab << "\"FREQUENCY_MAX\" : 2300000000," << std::endl
               << tab << "\"FREQUENCY_MIN\" : 1200000000," << std::endl
               << tab << "\"PI\" : 3.14159265," << std::endl
               << tab << "\"DEFAULT1\" : \"NAN\"," << std::endl
               << tab << "\"DEFAULT2\" : \"nan\"," << std::endl
               << tab << "\"DEFAULT3\" : \"NaN\"" << std::endl
               << "}" << std::endl;
    m_valid_json = valid_json.str();

    std::ostringstream bad_json;
    bad_json << "{" << std::endl
             << tab << "\"POWER_MAX\" : 400," << std::endl
             << tab << "\"FREQUENCY_MAX\" : 2300000000," << std::endl
             << tab << "\"FREQUENCY_MIN\" : \"WUBBA LUBBA DUB DUB\"," << std::endl // Strings are not handled.
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

TEST_F(FilePolicyTest, parse_json_file)
{
    std::vector<std::string> signal_names = {"POWER_MAX", "FREQUENCY_MAX", "FREQUENCY_MIN", "PI",
                                             "DEFAULT1", "DEFAULT2", "DEFAULT3"};
    FilePolicy file_policy(m_json_file_path, signal_names);
    std::vector<double> result = file_policy.get_policy();
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
    std::vector<std::string> signal_names = {"POWER_MAX", "FREQUENCY_MAX", "FREQUENCY_MIN", "PI",
                                             "DEFAULT1", "DEFAULT2", "DEFAULT3"};
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy(m_json_file_path_bad, signal_names),
                               GEOPM_ERROR_FILE_PARSE, "unsupported type or malformed json config file");

    std::ofstream bad_json(m_json_file_path_bad);
    bad_json << "{" << std::endl
             << "\"FAKE_SIGNAL\" : 400," << std::endl
             << "\"FREQUENCY_MAX\" : 2300000000" << std::endl
             << "}" << std::endl;
    bad_json.close();
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy(m_json_file_path_bad, signal_names),
                               GEOPM_ERROR_INVALID, "invalid policy name");

    // Don't parse if Agent doesn't require any policies
    const std::vector<std::string> signal_names_empty;
    FilePolicy file_policy("", signal_names_empty);
    auto empty_result = file_policy.get_policy();
    EXPECT_EQ(0u, empty_result.size());
}

TEST_F(FilePolicyTest, negative_bad_files)
{
    std::string path ("FilePolicyTest_empty");
    std::ofstream empty_file(path, std::ofstream::out);
    empty_file.close();
    const std::vector<std::string> signal_names = {"FAKE_SIGNAL"};
    std::vector<double> policy {NAN};
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy(path, signal_names),
                               GEOPM_ERROR_INVALID, "input file invalid");
    chmod(path.c_str(), 0);
    GEOPM_EXPECT_THROW_MESSAGE(FilePolicy(path, signal_names),
                               EACCES, "file \"" + path + "\" could not be opened");
    unlink(path.c_str());
}
