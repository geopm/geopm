/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STATSCOLLECTOR_HPP_INCLUDE
#define STATSCOLLECTOR_HPP_INCLUDE

#include <vector>
#include <string>
#include <memory>

struct geopm_request_s;

namespace geopm
{
    class PlatformIO;
    class RuntimeStats;

    class StatsCollector
    {
        public:
            StatsCollector();
            StatsCollector(const std::vector<geopm_request_s> &requests);
            StatsCollector(const std::vector<geopm_request_s> &requests, PlatformIO &pio);
            virtual ~StatsCollector() = default;
            void update(void);
            std::string report_yaml(void) const;
            void reset(void);
        private:
            PlatformIO &m_pio;
            std::vector<std::string> register_requests(const std::vector<geopm_request_s> &requests);
            std::vector<std::string> m_metric_names;
            std::vector<int> m_pio_idx;
            std::shared_ptr<RuntimeStats> m_stats;
            std::string m_time_begin;
    };
}

#endif
