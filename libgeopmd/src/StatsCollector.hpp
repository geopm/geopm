/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STATSCOLLECTOR_HPP_INCLUDE
#define STATSCOLLECTOR_HPP_INCLUDE

#include <vector>
#include <string>
#include <memory>

#include "geopm/PlatformIO.hpp"

namespace geopm
{
    class RuntimeStats;

    /// @brief Class that accumulates statistics based on PlatformIO requests
    class StatsCollector
    {
        public:
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
        private:
            std::vector<std::string> register_requests(const std::vector<geopm_request_s> &requests);
            PlatformIO &m_pio;
            std::vector<std::string> m_metric_names;
            std::vector<int> m_pio_idx;
            std::shared_ptr<RuntimeStats> m_stats;
            int m_time_pio_idx;
            int m_sample_count; // Number of times m_pio.sample() has been called
            double m_time_sample;
            double m_time_delta_m_1;
            double m_time_delta_m_2;
            std::string m_time_begin_str;
            double m_time_begin;
    };
}

#endif
