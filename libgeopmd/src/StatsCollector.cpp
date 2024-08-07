/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "StatsCollector.hpp"

#include "geopm_stats_collector.h"

#include <sstream>
#include <cmath>
#include <cstring>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_time.h"
#include "RuntimeStats.hpp"

namespace geopm
{

    std::unique_ptr<StatsCollector> StatsCollector::make_unique(const std::vector<geopm_request_s> &requests)
    {
        return std::make_unique<StatsCollector>(requests);
    }


    StatsCollector::StatsCollector(const std::vector<geopm_request_s> &requests)
        : StatsCollector(requests, platform_io())
    {

    }

    StatsCollector::StatsCollector(const std::vector<geopm_request_s> &requests, PlatformIO &pio)
        : m_pio(pio)
        , m_stats(std::make_shared<RuntimeStats>(register_requests(requests)))
        , m_time_begin(geopm::time_curr_string())
    {

    }

    std::vector<std::string> StatsCollector::register_requests(const std::vector<geopm_request_s> &requests)
    {
        m_metric_names.clear();
        for (const auto &req : requests) {
            m_pio_idx.push_back(m_pio.push_signal(req.name, req.domain_type, req.domain_idx));
            if (req.domain_type == GEOPM_DOMAIN_BOARD && req.domain_idx == 0) {
                m_metric_names.push_back(req.name);
            }
            else {
                std::ostringstream name_stream;
                name_stream << req.name << "-" << platform_topo().domain_type_to_name(req.domain_type) << '-' << req.domain_idx;
                m_metric_names.push_back(name_stream.str());
            }
        }
        return m_metric_names;
    }

    void StatsCollector::update(void)
    {
        // Caller is expected to have called platform_io().read_batch();
        std::vector<double> sample(m_pio_idx.size(), 0.0);
        auto sample_it = sample.begin();
        for (auto pio_idx : m_pio_idx) {
            *sample_it = m_pio.sample(pio_idx);
            ++sample_it;
        }
        m_stats->update(sample);
    }

    std::string StatsCollector::report_yaml(void) const
    {
        std::ostringstream result;
        std::string time_end_str = geopm::time_curr_string();
        result << "hosts:\n";
        result << "  " << geopm::hostname() << ":\n";
        result << "    " << "time-begin: \"" << m_time_begin << "\"\n";
        result << "    " << "time-end: \"" <<  time_end_str << "\"\n";
        result << "    " << "metrics:\n";
        int metric_idx = 0;
        for (const auto &metric_name : m_metric_names) {
            result << "      " << metric_name << ":\n";
            result << "        " << "count: " << m_stats->count(metric_idx) << "\n";
            result << "        " << "first: " << m_stats->first(metric_idx) << "\n";
            result << "        " << "last: " << m_stats->last(metric_idx) << "\n";
            result << "        " << "min: " << m_stats->min(metric_idx) << "\n";
            result << "        " << "max: " << m_stats->max(metric_idx) << "\n";
            result << "        " << "mean: " << m_stats->mean(metric_idx) << "\n";
            result << "        " << "std: " << m_stats->std(metric_idx) << "\n";
            ++metric_idx;
        }
        return result.str();
    }

    void StatsCollector::reset(void)
    {
        m_stats->reset();
    }

}

int geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector)
{
    int err = 0;
    try {
        std::vector<geopm_request_s> request_vec(requests, requests + num_requests);
        auto result = geopm::StatsCollector::make_unique(request_vec);
        *collector = reinterpret_cast<geopm_stats_collector_s *>(result.release());
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_stats_collector_update(struct geopm_stats_collector_s *collector)
{
    int err = 0;
    try {
        geopm::StatsCollector *collector_cpp = reinterpret_cast<geopm::StatsCollector *>(collector);
        collector_cpp->update();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

// If *max_report_size is zero, update it with the required size for the report
// and do not modify report
int geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml)
{
    int err = 0;
    try {
        const geopm::StatsCollector *collector_cpp = reinterpret_cast<const geopm::StatsCollector *>(collector);
        std::string report_str = collector_cpp->report_yaml();
        if (report_str.size() >= *max_report_size) {
            *max_report_size = report_str.size() + 1;
            throw geopm::Exception("geopm_stats_collector_report(): max_report_size is too small",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        strncpy(report_yaml, report_str.c_str(), report_str.size());
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_stats_collector_reset(struct geopm_stats_collector_s *collector)
{
    int err = 0;
    try {
        geopm::StatsCollector *collector_cpp = reinterpret_cast<geopm::StatsCollector *>(collector);
        collector_cpp->reset();
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}

int geopm_stats_collector_free(geopm_stats_collector_s *collector)
{
    int err = 0;
    try {
        geopm::StatsCollector *collector_cpp = reinterpret_cast<geopm::StatsCollector *>(collector);
        delete collector_cpp;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}
