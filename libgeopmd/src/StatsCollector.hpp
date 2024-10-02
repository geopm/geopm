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
            struct report_s {
                std::string host;
                std::string sample_time_first;
                std::array<double, GEOPM_NUM_SAMPLE_STATS> sample_stats;
                std::vector<std::string> metric_names;
                std::vector<std::array<double, GEOPM_NUM_METRIC_STATS> > metric_stats;
            };

            /// @brief Factory access method
            ///
            /// User specifies a vector of PlatformIO signal requests to be
            /// accumulated.  The report will generate statistics about each
            /// signal request.
            ///
            /// @param [in] requests All signals for monitoring and reporting
            ///
            /// @return Unique pointer to StatCollector object
            static std::unique_ptr<StatsCollector> make_unique(const std::vector<geopm_request_s> &requests);
            /// @brief Default null constructor without requests
            StatsCollector() = default;
            /// @brief Default destructor
            virtual ~StatsCollector() = default;
            /// @brief Sample PlatformIO and update all tracked signals
            virtual void update(void) = 0;
            /// @brief Generate a YAML report of statistics
            ///
            /// Returns a YAML formatted report providing statisics about all
            /// samples gathered since object construction or since last call to
            /// reset().
            ///
            /// @return YAML report string that includes hostname, start date,
            ///         end date, and signal statistics
            virtual std::string report_yaml(void) const = 0;
            /// @brief May be called after report_yaml() to reset statistics
            ///
            /// Used to generate independent reports by clearing all gathered
            /// statistics and resetting the begin time.
            virtual void reset(void) = 0;
            /// @brief Return report of statistics in a structure representation
            ///
            /// Creates a report_s structure providing statistics about all
            /// samples gathered since object construction or since last call to
            /// reset().
            ///
            /// @return report_s structure containing vectors of metric_names
            ///         and metric_stats based on constructor requests.
            virtual report_s report_struct(void) const = 0;
            /// @brief Number of updates since last reset
            ///
            /// Returns the number of times the update() method has been called
            /// since object construction or last call to reset().
            ///
            /// @return Number of updates
            virtual size_t update_count(void) const = 0;
    };


    class StatsCollectorImp : public StatsCollector
    {
        public:
            /// @brief Default null constructor without requests
            StatsCollectorImp();
            /// @brief Standard constructor with requests
            ///
            /// User specifies a vector of PlatformIO signal requests to be
            /// accumulated.  The report will generate statistics about each
            /// signal request.
            ///
            /// @param [in] requests All signals for monitoring and reporting
            StatsCollectorImp(const std::vector<geopm_request_s> &requests);
            /// @brief Test constructor used to mock PlatformIO
            StatsCollectorImp(const std::vector<geopm_request_s> &requests, PlatformIO &pio);
            /// @brief Default destructor
            ~StatsCollectorImp() = default;
            void update(void) override;
            std::string report_yaml(void) const override;
            void reset(void) override;
            report_s report_struct(void) const override;
            size_t update_count(void) const override;
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
