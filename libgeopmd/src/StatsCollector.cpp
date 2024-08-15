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

    StatsCollector::StatsCollector()
        : StatsCollector(std::vector<geopm_request_s> {})
    {

    }

    StatsCollector::StatsCollector(const std::vector<geopm_request_s> &requests)
        : StatsCollector(requests, platform_io())
    {

    }

    StatsCollector::StatsCollector(const std::vector<geopm_request_s> &requests, PlatformIO &pio)
        : m_pio(pio)
        , m_stats(std::make_shared<RuntimeStats>(register_requests(requests)))
        , m_time_pio_idx(m_pio.push_signal("TIME", GEOPM_DOMAIN_BOARD, 0))
        , m_sample_count(0)
        , m_time_sample(0.0)
        , m_time_delta_m_1(0.0)
        , m_time_delta_m_2(0.0)
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
        double time_last = m_time_sample;
        m_time_sample = m_pio.sample(m_time_pio_idx);
        ++m_sample_count;
        if (m_time_begin_str == "") {
            m_time_begin = m_time_sample;
            double time_curr = m_pio.read_signal("TIME", GEOPM_DOMAIN_BOARD, 0);
            geopm_time_s time_curr_real = geopm::time_curr_real();
            geopm_time_s time_begin_real;
            geopm_time_add(&time_curr_real, m_time_sample - time_curr, &time_begin_real);
            char time_begin_cstr[NAME_MAX];
            int err = geopm_time_real_to_iso_string(&time_begin_real, NAME_MAX, time_begin_cstr);
            if (err != 0) {
                throw Exception("StatsCollector::update(): geopm_time_real_to_iso_string() call failed",
                                err, __FILE__, __LINE__);
            }
            m_time_begin_str = time_begin_cstr;
        }
        else {
            double time_delta = m_time_sample - time_last;
            m_time_delta_m_1 += time_delta;
            m_time_delta_m_2 += time_delta * time_delta;
        }
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
        double time_delta_mean = 0;
        double time_delta_std = 0;
        // Two samples are required to measure one time difference, so we must
        // have at least two samples to estimate the mean time difference.  One
        // degree of freedom is lost due to the differencing.
        if (m_sample_count > 1) {
            time_delta_mean = m_time_delta_m_1 / (m_sample_count - 1);
        }
        // Three samples are required to measure two time differences, so we
        // must have at least three samples to estimate the standard deviation
        // of the time difference.  One degree of freedom is lost due to the
        // differencing.
        if (m_sample_count > 2) {
            time_delta_std = std::sqrt(
                (m_time_delta_m_2 -
                 m_time_delta_m_1 *
                 m_time_delta_m_1 / (m_sample_count - 1)) /
                (m_sample_count - 2));
        }
        result << "host: \"" << geopm::hostname() << "\"\n";
        result << "sample-time-first: \"" << m_time_begin_str << "\"\n";
        result << "sample-time-total: " <<  m_time_sample - m_time_begin << "\n";
        result << "sample-count: " << m_sample_count << "\n";
        result << "sample-period-mean: " << time_delta_mean << "\n";
        result << "sample-period-std: " << time_delta_std << "\n";
        result << "metrics:\n";
        int metric_idx = 0;
        for (const auto &metric_name : m_metric_names) {
            result << "  " << metric_name << ":\n";
            result << "    " << "count: " << m_stats->count(metric_idx) << "\n";
            result << "    " << "first: " << m_stats->first(metric_idx) << "\n";
            result << "    " << "last: " << m_stats->last(metric_idx) << "\n";
            result << "    " << "min: " << m_stats->min(metric_idx) << "\n";
            result << "    " << "max: " << m_stats->max(metric_idx) << "\n";
            result << "    " << "mean-arithmetic: " << m_stats->mean(metric_idx) << "\n";
            result << "    " << "std: " << m_stats->std(metric_idx) << "\n";
            ++metric_idx;
        }
        return result.str();
    }

    void StatsCollector::reset(void)
    {
        m_time_begin_str = "";
        m_time_begin = 0;
        m_sample_count = 0;
        m_time_sample = 0.0;
        m_time_delta_m_1 = 0.0;
        m_time_delta_m_2 = 0.0;
        m_stats->reset();
    }

}

int geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector)
{
    int err = 0;
    try {
        std::vector<geopm_request_s> request_vec(requests, requests + num_requests);
        auto result = std::make_unique<geopm::StatsCollector>(request_vec);
        *collector = reinterpret_cast<geopm_stats_collector_s *>(result.release());
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
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
    }
    return err;
}

// If report_yaml == NULL and *max_report_size is zero, update it with the
// required size for the report and do not modify report
int geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml)
{
    int err = 0;
    try {
        const geopm::StatsCollector *collector_cpp = reinterpret_cast<const geopm::StatsCollector *>(collector);
        std::string report_str = collector_cpp->report_yaml();
        if (*max_report_size == 0 && report_yaml == nullptr) {
            // This call is querying the size of the buffer
            *max_report_size = report_str.size() + 1;
        }
        else if (report_str.size() < *max_report_size) {
            strncpy(report_yaml, report_str.c_str(), report_str.size() + 1);
        }
        else {
            err = ENOBUFS;
            std::ostringstream err_str;
            size_t report_str_size = report_str.size() + 1;
            err_str << "geopm_stats_collector_report_yaml(): max_report_size is too small, provided: "
                    << *max_report_size << " required: " << report_str_size;
            *max_report_size = report_str_size;
            throw geopm::Exception(err_str.str(), err, __FILE__, __LINE__);
        }
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
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
    }
    return err;
}

int geopm_stats_collector_free(struct geopm_stats_collector_s *collector)
{
    int err = 0;
    try {
        geopm::StatsCollector *collector_cpp = reinterpret_cast<geopm::StatsCollector *>(collector);
        delete collector_cpp;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
    }
    return err;
}
