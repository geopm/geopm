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

//TODO: Change from logits to probabilities
class RegionHintRecommenderTest : public ::testing::Test
{
    protected:
        std::string m_filename = "freq_map_test.json";
        void TearDown() override;
};

void RegionHintRecommenderTest::TearDown()
{
    std::remove(m_filename.c_str());
}

TEST_F(RegionHintRecommenderTest, test_json_parsing)
{
    {
        std::ofstream bad_json(m_filename);
        bad_json << "{[\"test\"]" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(m_filename, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

    {
        std::ofstream bad_json(m_filename);
        bad_json << "{\"A\": \"not this!\"}" << std::endl;
        bad_json.close();

        GEOPM_EXPECT_THROW_MESSAGE(
                        RegionHintRecommenderImp(m_filename, 0, 1),
                        GEOPM_ERROR_INVALID,
                        "Frequency map file format is incorrect");
    }

}

TEST_F(RegionHintRecommenderTest, test_plumbing)
{
    std::ofstream good_json(m_filename);
    good_json << "{\"A\": [0, 0.8, 0],"
                 "\"B\": [0, 1, 0, 1, 0],"
                 "\"C\": [0.3]}" << std::endl;
    good_json.close();

    RegionHintRecommenderImp hint_map(m_filename, 0, 1);

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
