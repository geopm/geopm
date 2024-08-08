/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RUNTIMESTATS_HPP_INCLUDE
#define RUNTIMESTATS_HPP_INCLUDE

#include <vector>
#include <string>

namespace geopm
{
    /// @brief Class that aggregates statistics without buffered data
    class RuntimeStats
    {
        public:
            /// @brief Default null RuntimeStats with no signals
            RuntimeStats() = default;
            /// @brief Constructor that records the names of all metrics
            RuntimeStats(const std::vector<std::string> &metric_names);
            /// @brief Default virtual destructor
            virtual ~RuntimeStats() = default;
            /// @brief Number of metrics aggregated
            ///
            /// @return The number of metrics specified at construction
            int num_metric(void) const;
            /// @brief Name of one metrics
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Name of metric
            std::string metric_name(int metric_idx) const;
            /// @brief Number of non-null values sampled
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Number of non-null samples
            uint64_t count(int metric_idx) const;
            /// @brief First non-null value sampled
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return First value of metric
            double first(int metric_idx) const;
            /// @brief Last non-null value sampled
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Last value of metric
            double last(int metric_idx) const;
            /// @brief Minimum value sampled
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Minimum value of metric
            double min(int metric_idx) const;
            /// @brief Maximum value sampled
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Maximum value of metric
            double max(int metric_idx) const;
            /// @brief Mean value sampled
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Mean value of metric
            double mean(int metric_idx) const;
            /// @brief Estimate of standard deviation
            ///
            /// @param [in] metric_idx Index of the metric as specified at
            ///             construction
            ///
            /// @return Standard deviation estimate of metric
            double std(int metric_idx) const;
            /// @brief Reset all aggregated statistics
            void reset(void);
            /// @brief Update statistics with new sample
            ///
            /// Update all metrics being aggregated.
            ///
            /// @param [in] sample Vector of sample values: one for each metric
            ///        name specified at construction
            void update(const std::vector<double> &sample);
        private:
            void check_index(int metric_idx, const std::string &func, int line) const;
            struct stats_s {
                stats_s &operator=(const stats_s &other);
                uint64_t count;
                double first;
                double last;
                double min;
                double max;
                double m_1;
                double m_2;
            };
            const std::vector<std::string> m_metric_names;
            std::vector<stats_s> m_metric_stats;
    };
}

#endif
