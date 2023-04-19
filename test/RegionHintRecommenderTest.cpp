/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <cmath>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

#include "RegionHintRecommender.hpp"
#include "RegionHintRecommenderImp.hpp"

using geopm::RegionHintRecommender;
using geopm::RegionHintRecommenderImp;
using ::testing::_;

class RegionHintRecommenderTest : public ::testing::Test
{
    protected:
        const std::string M_FILENAME = "freq_map_test.json";
        void TearDown() override;
};

void RegionHintRecommenderTest::TearDown()
{
    std::remove(M_FILENAME.c_str());
}

TEST_F(RegionHintRecommenderTest, test_json_parsing)
{
    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{[\"test\"]" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{ }" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "must contain a frequency map");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{\"A\": \"not this!\"}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{\"A\": 5.0}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{\"A\": []}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "[1, 2, 4]" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(M_FILENAME);
        bad_json << "{\"A\": [\"a\", \"b\", \"c\"]}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(M_FILENAME, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Non-numeric value found");
    }
}

TEST_F(RegionHintRecommenderTest, test_plumbing)
{
    std::ofstream good_json(M_FILENAME);
    good_json << "{\"A\": [0, 0.8, 0],"
                 "\"B\": [0, 1, 0, 1, -3],"
                 "\"C\": [0.3]}" << std::endl;
    good_json.close();

    RegionHintRecommenderImp hint_map(M_FILENAME, 0, 1);

    EXPECT_EQ(hint_map.recommend_frequency({{"A", 1}}, 0), 0);
    EXPECT_EQ(hint_map.recommend_frequency({{"A", 1}}, 0.25), 0);
    EXPECT_EQ(hint_map.recommend_frequency({{"A", 1}}, 0.5), 8e7);
    EXPECT_EQ(hint_map.recommend_frequency({{"A", 1}}, 0.75), 8e7);
    EXPECT_EQ(hint_map.recommend_frequency({{"A", 1}}, 1), 0);

    EXPECT_EQ(hint_map.recommend_frequency({{"B", 1}}, 0), 0);
    EXPECT_EQ(hint_map.recommend_frequency({{"B", 1}}, 0.25), 1e8);
    EXPECT_EQ(hint_map.recommend_frequency({{"B", 1}}, 0.5), 0);
    EXPECT_EQ(hint_map.recommend_frequency({{"B", 1}}, 0.75), 1e8);
    EXPECT_EQ(hint_map.recommend_frequency({{"B", 1}}, 1), 0);

    EXPECT_EQ(hint_map.recommend_frequency({{"C", 1}}, 0), 3e7);
    EXPECT_EQ(hint_map.recommend_frequency({{"C", 1}}, 0.25), 3e7);
    EXPECT_EQ(hint_map.recommend_frequency({{"C", 1}}, 0.5), 3e7);
    EXPECT_EQ(hint_map.recommend_frequency({{"C", 1}}, 0.75), 3e7);
    EXPECT_EQ(hint_map.recommend_frequency({{"C", 1}}, 1), 3e7);

    EXPECT_NEAR(hint_map.recommend_frequency({{"A", log(2)}, {"B", 0}, {"C", log(0.5)}}, 0.5), 5e7, 1);
}
