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
    class RuntimeStats
    {
        public:
            RuntimeStats() = default;
            RuntimeStats(const std::vector<std::string> &metric_names);
            virtual ~RuntimeStats() = default;
            int num_metric(void) const;
            std::string metric_name(int metric_idx) const;
            uint64_t count(int metric_idx) const;
            double first(int metric_idx) const;
            double last(int metric_idx) const;
            double min(int metric_idx) const;
            double max(int metric_idx) const;
            double mean(int metric_idx) const;
            double std(int metric_idx) const;
            double skew(int metric_idx) const;
            double kurt(int metric_idx) const;
            double lse_linear_0(int metric_idx) const;
            double lse_linear_1(int metric_idx) const;
            void reset(void);
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
                double m_3;
                double m_4;
            };
            const std::vector<std::string> m_metric_names;
            std::vector<stats_s> m_moments;
    };
}

#endif
