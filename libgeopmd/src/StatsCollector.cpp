/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm_stats_collector.h"

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <cmath>
#include <string.h>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include "geopm/PlatformIO.hpp"
#include "geopm/PlatformTopo.hpp"
#include "geopm_time.h"


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

    class RuntimeStatsCollector
    {
        public:
            static std::unique_ptr<RuntimeStatsCollector> make_unique(const std::vector<geopm_request_s> &requests);
            RuntimeStatsCollector() = default;
            virtual ~RuntimeStatsCollector() = default;
            RuntimeStatsCollector(const std::vector<geopm_request_s> &requests);
            void update(void);
            std::string report_yaml(void) const;
            void reset(void);
        private:
            std::vector<std::string> register_requests(const std::vector<geopm_request_s> &requests);
            std::vector<std::string> m_metric_names;
            std::vector<int> m_pio_idx;
            std::shared_ptr<RuntimeStats> m_stats;
            std::string m_time_begin;
    };

    std::unique_ptr<RuntimeStatsCollector> RuntimeStatsCollector::make_unique(const std::vector<geopm_request_s> &requests)
    {
        return std::make_unique<RuntimeStatsCollector>(requests);
    }

    RuntimeStatsCollector::RuntimeStatsCollector(const std::vector<geopm_request_s> &requests)
        : m_stats(std::make_shared<RuntimeStats>(register_requests(requests)))
    {

    }

    std::vector<std::string> RuntimeStatsCollector::register_requests(const std::vector<geopm_request_s> &requests)
    {
        m_metric_names.clear();
        for (const auto &req : requests) {
            m_pio_idx.push_back(platform_io().push_signal(req.name, req.domain_type, req.domain_idx));
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

    void RuntimeStatsCollector::update(void)
    {
        if (m_time_begin.size() == 0) {
            geopm_time_s time_begin = geopm::time_curr_real();
            char time_begin_str[NAME_MAX];
            int err = geopm_time_real_to_iso_string(&time_begin, NAME_MAX, time_begin_str);
            if (err != 0) {
                throw Exception("RuntimeStatsCollector::update(): Failed to convert time string",
                                err, __FILE__, __LINE__);
            }
            m_time_begin = time_begin_str;
        }
        // Caller is expected to have called platform_io().read_batch();
        std::vector<double> sample(m_pio_idx.size(), 0.0);
        auto sample_it = sample.begin();
        for (auto pio_idx : m_pio_idx) {
            *sample_it = platform_io().sample(pio_idx);
            ++sample_it;
        }
        m_stats->update(sample);
    }

    std::string RuntimeStatsCollector::report_yaml(void) const
    {
        std::ostringstream result;
        geopm_time_s time_end = geopm::time_curr_real();
        char time_end_str[NAME_MAX];
        int err = geopm_time_real_to_iso_string(&time_end, NAME_MAX, time_end_str);
        if (err != 0) {
            throw Exception("RuntimeStatsCollector::report_yaml(): Failed to convert time string",
                            err, __FILE__, __LINE__);
        }
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

    void RuntimeStatsCollector::reset(void)
    {
        m_stats->reset();
    }

    RuntimeStats::RuntimeStats(const std::vector<std::string> &metric_names)
        : m_metric_names(metric_names)
        , m_moments(m_metric_names.size())
    {
        reset();
    }

    int RuntimeStats::num_metric(void) const
    {
        return m_metric_names.size();
    }

    void RuntimeStats::check_index(int metric_idx, const std::string &func, int line) const
    {
        if (metric_idx < 0 || (size_t)metric_idx >= m_metric_names.size()) {
            throw Exception("RuntimeStats::" + func  + "(): metric_idx out of range: " + std::to_string(metric_idx),
                            GEOPM_ERROR_INVALID, __FILE__, line);
        }
    }

    std::string RuntimeStats::metric_name(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        return m_metric_names[metric_idx];
    }

    uint64_t RuntimeStats::count(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        return m_moments[metric_idx].count;
    }

    double RuntimeStats::first(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].first;
        }
        return result;
    }

    double RuntimeStats::last(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].last;
        }
        return result;
    }

    double RuntimeStats::min(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].min;
        }
        return result;
    }

    double RuntimeStats::max(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].max;
        }
        return result;
    }

    double RuntimeStats::mean(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count != 0) {
            result = m_moments[metric_idx].m_1 /
                     m_moments[metric_idx].count;
        }
        return result;
    }

    double RuntimeStats::std(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_moments[metric_idx].count > 1) {
            result = std::sqrt(
                         (m_moments[metric_idx].m_2 -
                          m_moments[metric_idx].m_1 *
                          m_moments[metric_idx].m_1 / m_moments[metric_idx].count) /
                         (m_moments[metric_idx].count - 1));
        }
        return result;
    }

    double RuntimeStats::skew(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double RuntimeStats::kurt(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double RuntimeStats::lse_linear_0(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    double RuntimeStats::lse_linear_1(int metric_idx) const
    {
        throw Exception("RuntimeStats::" + std::string(__func__) + " not yet implemented",
                        GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
    }

    void RuntimeStats::reset(void)
    {
        for (auto &it : m_moments) {
            it.count = 0;
            it.m_1 = 0.0;
            it.m_2 = 0.0;
            it.m_3 = 0.0;
            it.m_4 = 0.0;
        }
    }

    void RuntimeStats::update(const std::vector<double> &sample)
    {
        if (sample.size() != m_moments.size()) {
            throw Exception("RuntimeStats::update(): invalid input vector size: " + std::to_string(sample.size()),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto moments_it = m_moments.begin();
        for (const auto &ss : sample) {
            if (platform_io().is_valid_value(ss)) {
                moments_it->count += 1;
                if (moments_it->count == 1) {
                    moments_it->first = ss;
                    moments_it->min = ss;
                    moments_it->max = ss;
                }
                moments_it->last = ss;
                if (moments_it->min > ss) {
                    moments_it->min = ss;
                }
                if (moments_it->max < ss) {
                    moments_it->max = ss;
                }
                double mm = ss;
                moments_it->m_1 += mm;
                mm *= ss;
                moments_it->m_2 += mm;
                mm *= ss;
                moments_it->m_3 += mm;
                mm *= ss;
                moments_it->m_4 += mm;
            }
            ++moments_it;
        }
    }
}

int geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector)
{
    int err = 0;
    try {
        std::vector<geopm_request_s> request_vec(requests, requests + num_requests);
        auto result = geopm::RuntimeStatsCollector::make_unique(request_vec);
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
        geopm::RuntimeStatsCollector *collector_cpp = reinterpret_cast<geopm::RuntimeStatsCollector *>(collector);
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
        const geopm::RuntimeStatsCollector *collector_cpp = reinterpret_cast<const geopm::RuntimeStatsCollector *>(collector);
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
        geopm::RuntimeStatsCollector *collector_cpp = reinterpret_cast<geopm::RuntimeStatsCollector *>(collector);
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
        geopm::RuntimeStatsCollector *collector_cpp = reinterpret_cast<geopm::RuntimeStatsCollector *>(collector);
        delete collector_cpp;
    }
    catch (...) {
        err = geopm::exception_handler(std::current_exception());
        err = err < 0 ? err : GEOPM_ERROR_RUNTIME;
    }
    return err;
}
