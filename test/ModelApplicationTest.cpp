/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019 Intel Corporation
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

#include <cstdio>
#include <limits.h>
#include <fstream>
#include <string>
#include <vector>

#include "gtest/gtest.h"

#include "ModelParse.hpp"
#include "Exception.hpp"

using geopm::model_parse_config;
using geopm::Exception;


class ModelApplicationTest : public ::testing::Test
{
    protected:
        virtual void TearDown();

        std::string m_filename = "model_application_test.json";
        // variables for output
        uint64_t m_loop_count = 0;
        std::vector<std::string> m_region_name;
        std::vector<double> m_big_o;
};

void ModelApplicationTest::TearDown()
{
    std::remove(m_filename.c_str());
}

TEST_F(ModelApplicationTest, parse_config_errors)
{
    // no file, empty file
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);
    std::ofstream empty_file(m_filename);
    empty_file.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // malformed file
    std::ofstream bad_json(m_filename);
    bad_json << "{[\"test\"]" << std::endl;
    bad_json.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // unknown key
    std::ofstream unknown_key(m_filename);
    unknown_key << "{\"unknown\":1}" << std::endl;
    unknown_key.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // loop count must be integer
    std::ofstream bad_loop_count(m_filename);
    bad_loop_count << "{\"loop-count\":\"one\"}" << std::endl;
    bad_loop_count.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    std::ofstream bad_loop_count2(m_filename);
    bad_loop_count2 << "{\"loop-count\":22.2}" << std::endl;
    bad_loop_count2.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // region must be array
    std::ofstream bad_region(m_filename);
    bad_region << "{\"region\":\"myregion\"}" << std::endl;
    bad_region.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // region array items must be string
    std::ofstream bad_region_item(m_filename);
    bad_region_item << "{\"region\":[22]}" << std::endl;
    bad_region_item.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // big-o must be array
    std::ofstream bad_bigo(m_filename);
    bad_bigo << "{\"big-o\":\"biggo\"}" << std::endl;
    bad_bigo.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // big-o items must be double
    std::ofstream bad_bigo_item(m_filename);
    bad_bigo_item << "{\"big-o\":[\"number\"]}" << std::endl;
    bad_bigo_item.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // hostname must be array
    std::ofstream bad_hostname(m_filename);
    bad_hostname << "{\"hostname\":\"myhost\"}" << std::endl;
    bad_hostname.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // hostname items must be string
    std::ofstream bad_hostname_item(m_filename);
    bad_hostname_item << "{\"hostname\":[123]}" << std::endl;
    bad_hostname_item.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // imbalance must be array
    std::ofstream bad_imbalance(m_filename);
    bad_imbalance << "{\"imbalance\":\"ecnalabmi\"}" << std::endl;
    bad_imbalance.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // imbalance values must be double, non-negative
    std::ofstream bad_imbalance_item(m_filename);
    bad_imbalance_item << "{\"imbalance\":[\"hello\"]}" << std::endl;
    bad_imbalance_item.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    char hostname_tmp[NAME_MAX];
    hostname_tmp[NAME_MAX - 1] = '\0';
    if (gethostname(hostname_tmp, NAME_MAX - 1)) {
        FAIL() << "unable to get hostname";
    }
    std::ofstream bad_imbalance_item2(m_filename);
    bad_imbalance_item2 << "{\"imbalance\":[-20.2], \"hostname\":[\""
                        << hostname_tmp << "\"]}" << std::endl;
    bad_imbalance_item2.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    // region, big-o, hostname, and imbalance arrays must be same length
    std::ofstream mismatch_region_length(m_filename);
    mismatch_region_length << "{\"region\":[\"one\", \"two\"], \"big-o\":[2.2]}" << std::endl;
    mismatch_region_length.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
                 Exception);

    std::ofstream mismatch_hostname_length(m_filename);
    mismatch_hostname_length << "{\"hostname\":[\"one\", \"two\"], \"imbalance\":[2.2]}" << std::endl;
    mismatch_hostname_length.close();
    EXPECT_THROW(model_parse_config(m_filename, m_loop_count, m_region_name, m_big_o),
    Exception);
}
