/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "StatsCollector.hpp"
#include "geopm_stats_collector.h"

#include <memory>
#include <stdlib.h>
#include <errno.h>
#include <cmath>

#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"
#include "MockPlatformIO.hpp"
#include "MockStatsCollector.hpp"

using geopm::StatsCollectorImp;
using testing::Return;

class StatsCollectorTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::shared_ptr<MockPlatformIO> m_pio_mock;
};

void StatsCollectorTest::SetUp()
{
    m_pio_mock = std::make_shared<MockPlatformIO>();
}

/// @brief test report generation with no updates
TEST_F(StatsCollectorTest, empty_report)
{
    std::vector<geopm_request_s> req;
    EXPECT_CALL(*m_pio_mock, push_signal("TIME", 0, 0))
        .WillOnce(Return(0));
    auto coll = StatsCollectorImp(req, *m_pio_mock);
    std::string report = coll.report_yaml();
    std::vector<std::string> expected_begin = {
        "host",
        "sample-time-first",
        "sample-time-total",
        "sample-count",
        "sample-period-mean",
        "sample-period-std",
        "metrics",
    };
    auto eb_it = expected_begin.begin();
    for (const auto &line : geopm::string_split(report, "\n")) {
        if (eb_it == expected_begin.end()) {
            break;
        }
        auto line_split = geopm::string_split(line, ":");
        EXPECT_EQ(*eb_it, line_split[0]);
        ++eb_it;
    }
}

/// @brief Create two reports with a restart between
TEST_F(StatsCollectorTest, time_report)
{
    int pio_idx = 3;
    EXPECT_CALL(*m_pio_mock, push_signal("TIME", 0, 0))
        .WillOnce(Return(pio_idx))
        .WillOnce(Return(pio_idx));
    EXPECT_CALL(*m_pio_mock, read_signal("TIME", 0, 0))
        .WillOnce(Return(0.0))
        .WillOnce(Return(1.0));
    EXPECT_CALL(*m_pio_mock, sample(pio_idx))
        .WillOnce(Return(0.0))
        .WillOnce(Return(0.0))
        .WillOnce(Return(1.0))
        .WillOnce(Return(1.0))
        .WillOnce(Return(0.0))
        .WillOnce(Return(0.0))
        .WillOnce(Return(1.0))
        .WillOnce(Return(1.0));
    std::vector<geopm_request_s> req {geopm_request_s{0, 0, "TIME"}};
    auto coll = StatsCollectorImp(req, *m_pio_mock);
    std::vector<std::string> expected_begin = {
        "host",
        "sample-time-first",
        "sample-time-total",
        "sample-count",
        "sample-period-mean",
        "sample-period-std",
        "metrics",
        "  TIME",
        "    count",
        "    first",
        "    last",
        "    min",
        "    max",
        "    mean",
        "    std",
    };

    // Check the C++ interfaces
    coll.update();
    coll.update();
    std::string report = coll.report_yaml();
    auto eb_it = expected_begin.begin();
    for (const auto &line : geopm::string_split(report, "\n")) {
        if (eb_it == expected_begin.end()) {
            break;
        }
        auto line_split = geopm::string_split(line, ":");
        EXPECT_EQ(*eb_it, line_split[0]);
        ++eb_it;
    }
    EXPECT_NE(std::string::npos, report.find("count: 2\n"));
    EXPECT_NE(std::string::npos, report.find("first: 0\n"));
    EXPECT_NE(std::string::npos, report.find("last: 1\n"));
    EXPECT_NE(std::string::npos, report.find("min: 0\n"));
    EXPECT_NE(std::string::npos, report.find("max: 1\n"));
    EXPECT_NE(std::string::npos, report.find("mean: 0.5\n"));
    EXPECT_NE(std::string::npos, report.find("std: 0.707107\n"));
    struct geopm_stats_collector_s *coll_ptr = reinterpret_cast<geopm_stats_collector_s *>(&coll);
    EXPECT_EQ(0, geopm_stats_collector_reset(coll_ptr));
    EXPECT_EQ(0, geopm_stats_collector_update(coll_ptr));
    EXPECT_EQ(0, geopm_stats_collector_update(coll_ptr));
    size_t max_size = 0;
    ASSERT_EQ(0, geopm_stats_collector_report_yaml(coll_ptr, &max_size, nullptr));
    ASSERT_NE(0ULL, max_size);
    char *report_cstr = (char *)malloc(max_size);
    ASSERT_NE(nullptr, report_cstr);
    ASSERT_EQ(0, geopm_stats_collector_report_yaml(coll_ptr, &max_size, report_cstr));
    --max_size;
    ASSERT_EQ(ENOBUFS, geopm_stats_collector_report_yaml(coll_ptr, &max_size, report_cstr));
    report = report_cstr;
    free(report_cstr);
    eb_it = expected_begin.begin();
    for (const auto &line : geopm::string_split(report, "\n")) {
        if (eb_it == expected_begin.end()) {
            break;
        }
        auto line_split = geopm::string_split(line, ":");
        EXPECT_EQ(*eb_it, line_split[0]);
        ++eb_it;
    }
    EXPECT_NE(std::string::npos, report.find("count: 2\n"));
    EXPECT_NE(std::string::npos, report.find("first: 0\n"));
    EXPECT_NE(std::string::npos, report.find("last: 1\n"));
    EXPECT_NE(std::string::npos, report.find("min: 0\n"));
    EXPECT_NE(std::string::npos, report.find("max: 1\n"));
    EXPECT_NE(std::string::npos, report.find("mean: 0.5\n"));
    EXPECT_NE(std::string::npos, report.find("std: 0.707107\n"));

    EXPECT_EQ(2ULL, coll.update_count());
    size_t update_count = 0;
    EXPECT_EQ(0, geopm_stats_collector_update_count(coll_ptr, &update_count));
    EXPECT_EQ(2ULL, update_count);

    auto report_struct = coll.report_struct();
    geopm_report_s report_struct_c;
    report_struct_c.metric_stats = (struct geopm_metric_stats_s *)malloc(sizeof(geopm_metric_stats_s));
    size_t num_metric = 1;
    EXPECT_EQ(GEOPM_ERROR_INVALID, geopm_stats_collector_report(coll_ptr, 0, &report_struct_c));
    EXPECT_EQ(0, geopm_stats_collector_report(coll_ptr, num_metric, &report_struct_c));
    EXPECT_EQ(0, geopm_stats_collector_report(coll_ptr, num_metric, &report_struct_c));
    EXPECT_EQ(geopm::hostname(), report_struct.host);
    EXPECT_EQ(report_struct.host, report_struct_c.host);
    EXPECT_NE("", report_struct.sample_time_first);
    EXPECT_EQ(report_struct_c.sample_time_first, report_struct.sample_time_first);
    EXPECT_LT(0.0, report_struct.sample_stats[GEOPM_SAMPLE_TIME_TOTAL]);
    EXPECT_EQ(2.0, report_struct.sample_stats[GEOPM_SAMPLE_COUNT]);
    EXPECT_LT(0.0, report_struct.sample_stats[GEOPM_SAMPLE_PERIOD_MEAN]);
    EXPECT_EQ(0.0, report_struct.sample_stats[GEOPM_SAMPLE_PERIOD_STD]);
    for (int stat_idx = 0; stat_idx != GEOPM_NUM_SAMPLE_STATS; ++stat_idx) {
        EXPECT_EQ(report_struct.sample_stats[stat_idx], report_struct_c.sample_stats[stat_idx]);
    }
    ASSERT_EQ(1ULL, report_struct.metric_names.size());
    ASSERT_EQ(1ULL, report_struct.metric_stats.size());
    ASSERT_EQ(1ULL, report_struct_c.num_metric);
    EXPECT_EQ("TIME", report_struct.metric_names[0]);
    EXPECT_EQ("TIME", std::string(report_struct_c.metric_stats[0].name));
    EXPECT_EQ(2.0, report_struct.metric_stats[0][GEOPM_METRIC_COUNT]);
    EXPECT_EQ(0.0, report_struct.metric_stats[0][GEOPM_METRIC_FIRST]);
    EXPECT_EQ(1.0, report_struct.metric_stats[0][GEOPM_METRIC_LAST]);
    EXPECT_EQ(0.0, report_struct.metric_stats[0][GEOPM_METRIC_MIN]);
    EXPECT_EQ(1.0, report_struct.metric_stats[0][GEOPM_METRIC_MAX]);
    EXPECT_EQ(0.5, report_struct.metric_stats[0][GEOPM_METRIC_MEAN]);
    EXPECT_NEAR(1e-8, std::sqrt(2) / 2, report_struct.metric_stats[0][GEOPM_METRIC_STD]);
    for (size_t stat_idx = 0; stat_idx != GEOPM_NUM_METRIC_STATS; ++stat_idx) {
        EXPECT_EQ(report_struct.metric_stats[0][stat_idx], report_struct_c.metric_stats[0].stats[stat_idx]);
    }
    free(report_struct_c.metric_stats);
}

TEST_F(StatsCollectorTest, c_strings)
{
    MockStatsCollector mock_coll;
    std::vector<char> too_big(NAME_MAX, '*');
    too_big.push_back('\0');
    std::string too_big_str(too_big.data());
    geopm::StatsCollector::report_s too_big_report {
        too_big_str, too_big_str, {0.0, 0.0, 0.0, 0.0},
        {too_big_str}, {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}
    };
    EXPECT_CALL(mock_coll, report_struct()).WillOnce(Return(too_big_report));
    struct geopm_report_s report_c;
    report_c.metric_stats = (geopm_metric_stats_s *)malloc(sizeof(geopm_metric_stats_s));
    EXPECT_EQ(-1, geopm_stats_collector_report((geopm_stats_collector_s *)(&mock_coll), 1, &report_c));
    std::string max_str(too_big.data() + 1);
    geopm::StatsCollector::report_s max_report {
        max_str, max_str, {0.0, 0.0, 0.0, 0.0},
        {max_str}, {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}
    };
    EXPECT_CALL(mock_coll, report_struct()).WillOnce(Return(max_report));
    EXPECT_EQ(0, geopm_stats_collector_report((geopm_stats_collector_s *)(&mock_coll), 1, &report_c));
    EXPECT_EQ('*', report_c.host[NAME_MAX-2]);
    EXPECT_EQ('\0', report_c.host[NAME_MAX-1]);
    free(report_c.metric_stats);

    geopm::StatsCollector::report_s mixed_report {
        max_str, too_big_str, {0.0, 0.0, 0.0, 0.0},
        {max_str}, {{0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}}
    };
    EXPECT_CALL(mock_coll, report_struct()).WillOnce(Return(mixed_report));
    EXPECT_EQ(-1, geopm_stats_collector_report((geopm_stats_collector_s *)(&mock_coll), 1, &report_c));
    mixed_report.sample_time_first = max_str;
    mixed_report.metric_names[0] = too_big_str;
    EXPECT_CALL(mock_coll, report_struct()).WillOnce(Return(mixed_report));
    EXPECT_EQ(-1, geopm_stats_collector_report((geopm_stats_collector_s *)(&mock_coll), 1, &report_c));
}
