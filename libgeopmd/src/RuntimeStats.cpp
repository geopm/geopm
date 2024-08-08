/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RuntimeStats.hpp"

#include <cmath>

#include "geopm/Exception.hpp"

namespace geopm
{

    RuntimeStats::RuntimeStats(const std::vector<std::string> &metric_names)
        : m_metric_names(metric_names)
        , m_metric_stats(m_metric_names.size())
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
        return m_metric_stats[metric_idx].count;
    }

    double RuntimeStats::first(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_metric_stats[metric_idx].count != 0) {
            result = m_metric_stats[metric_idx].first;
        }
        return result;
    }

    double RuntimeStats::last(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_metric_stats[metric_idx].count != 0) {
            result = m_metric_stats[metric_idx].last;
        }
        return result;
    }

    double RuntimeStats::min(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_metric_stats[metric_idx].count != 0) {
            result = m_metric_stats[metric_idx].min;
        }
        return result;
    }

    double RuntimeStats::max(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_metric_stats[metric_idx].count != 0) {
            result = m_metric_stats[metric_idx].max;
        }
        return result;
    }

    double RuntimeStats::mean(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_metric_stats[metric_idx].count != 0) {
            result = m_metric_stats[metric_idx].m_1 /
                     m_metric_stats[metric_idx].count;
        }
        return result;
    }

    double RuntimeStats::std(int metric_idx) const
    {
        check_index(metric_idx, __func__, __LINE__);
        double result = NAN;
        if (m_metric_stats[metric_idx].count > 1) {
            result = std::sqrt(
                         (m_metric_stats[metric_idx].m_2 -
                          m_metric_stats[metric_idx].m_1 *
                          m_metric_stats[metric_idx].m_1 / m_metric_stats[metric_idx].count) /
                         (m_metric_stats[metric_idx].count - 1));
        }
        return result;
    }

    void RuntimeStats::reset(void)
    {
        for (auto &it : m_metric_stats) {
            it.count = 0;
            it.m_1 = 0.0;
            it.m_2 = 0.0;
        }
    }

    void RuntimeStats::update(const std::vector<double> &sample)
    {
        if (sample.size() != m_metric_stats.size()) {
            throw Exception("RuntimeStats::update(): invalid input vector size: " + std::to_string(sample.size()),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        auto moments_it = m_metric_stats.begin();
        for (const auto &ss : sample) {
            if (!std::isnan(ss)) {
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
            }
            ++moments_it;
        }
    }
}
