/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STATSCOLLECTOR_HPP_INCLUDE
#define STATSCOLLECTOR_HPP_INCLUDE

#include <vector>
#include <array>
#include <string>
#include <memory>

#include "geopm/PlatformIO.hpp"
#include "geopm_stats_collector.h"

namespace geopm
{
    class RuntimeStats;

    /// @brief Class that accumulates statistics based on PlatformIO requests
    class StatsCollector
    {
        public:
            enum sample_stats_e {
                SAMPLE_TIME_TOTAL,
                SAMPLE_COUNT,
                SAMPLE_PERIOD_MEAN,
                SAMPLE_PERIOD_STD,
                NUM_SAMPLE_STATS,
            };

            enum metric_stats_e {
                METRIC_COUNT,
                METRIC_FIRST,
                METRIC_LAST,
                METRIC_MIN,
                METRIC_MAX,
                METRIC_MEAN_ARITHMETIC,
                METRIC_STD,
                NUM_METRIC_STATS,
            };

            struct report_s {
                std::string host;
                std::string sample_time_first;
                std::array<double, NUM_SAMPLE_STATS> sample_stats;
                std::vector<std::string> metric_names;
                std::vector<std::array<double, NUM_METRIC_STATS> > metric_stats;
            };

            /// @brief Default null constructor without requests
            StatsCollector();
            /// @brief Standard constructor with requests
            ///
            /// User specifies a vector of PlatformIO signal requests to be
            /// accumulated.  The report will generate statistics about each
            /// signal request.
            ///
            /// @param [in] requests All signals for monitoring and reporting
            StatsCollector(const std::vector<geopm_request_s> &requests);
            /// @brief Test constructor used to mock PlatformIO
            StatsCollector(const std::vector<geopm_request_s> &requests, PlatformIO &pio);
            /// @brief Default destructor
            ~StatsCollector() = default;
            /// @brief Sample PlatformIO and update all tracked signals
            void update(void);
            /// @brief Generate a YAML report of statistics
            ///
            /// Returns a YAML formatted report providing statisics about all
            /// samples gathered since object construction or since last call to
            /// reset().
            ///
            /// @return YAML report string that includes hostname, start date,
            ///         end date, and signal statistics
            std::string report_yaml(void) const;
            /// @brief May be called after report_yaml() to reset statistics
            ///
            /// Used to generate independent reports by clearing all gathered
            /// statistics and resetting the begin time.
            void reset(void);
            /// @brief Return report of statistics in a structure representation
            ///
            /// Creates a report_s structure providing statistics about all
            /// samples gathered since object construction or since last call to
            /// reset().
            ///
            /// @return report_s structure containing vectors of metric_names
            ///         and metric_stats based on constructor requests.
            report_s report_struct(void) const;
            /// @brief Number of updates since last reset
            ///
            /// Returns the number of times the update() method has been called
            /// since object construction or last call to reset().
            ///
            /// @return Number of updates
            size_t update_count(void) const;
        private:
            std::vector<std::string> register_requests(const std::vector<geopm_request_s> &requests);
            std::string report_yaml_curr(void) const;
            PlatformIO &m_pio;
            std::vector<std::string> m_metric_names;
            std::vector<int> m_pio_idx;
            std::shared_ptr<RuntimeStats> m_stats;
            int m_time_pio_idx;
            size_t m_update_count; // Number of times update() has been called
            double m_time_sample;
            double m_time_delta_m_1;
            double m_time_delta_m_2;
            std::string m_time_begin_str;
            double m_time_begin;
            mutable bool m_is_cached;
            mutable std::string m_report_cache;
    };
}

#endif
