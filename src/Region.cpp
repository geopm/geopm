/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <string.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>

#include "Region.hpp"
#include "config.h"

namespace geopm
{
    Region::Region(uint64_t identifier, int level, TelemetryConfig &config)
        : m_identifier(identifier)
        , m_level(level)
        , m_entry_timestamp({{0, 0}})
        , m_num_entry(0)
        , m_mpi_time(0.0)
        , m_last_runtime(0.0)
        , m_agg_runtime(0.0)
        , m_is_runtime_requested(false)
        , m_runtime_slot(-1)
        , m_num_total_signal(0)
    {
        m_time_buffer = new CircularBuffer<struct geopm_time_s>(M_NUM_SAMPLE_HISTORY);
        if (!level) { //leaf level
            config.num_signal_per_domain(m_num_sig_domain); 
            m_domain_buffer.resize(m_num_sig_domain.size());
            m_domain_size.resize(m_num_sig_domain.size());
            for (auto it = m_domain_buffer.begin(); it != m_domain_buffer.end(); ++it) {
                (*it) = new CircularBuffer<std::vector<double> >(M_NUM_SAMPLE_HISTORY);
            }
            config.get_required(m_signal);
            config.get_aggregate(m_aggregate_signal); 
            m_curr_sample.resize(m_aggregate_signal.size(), 0.0);
            m_agg_stat.resize(m_aggregate_signal.size());
            int offset = 0;
            int slot = 0;
            auto agg_desc_it = m_agg_desc.begin();
            auto num_sig_it = m_num_sig_domain.begin();
            auto domain_size_it = m_domain_size.begin();
            // iterate over signals to be aggregated.
            for (auto agg_it = m_aggregate_signal.begin(); agg_it != m_aggregate_signal.end(); ++agg_it) {
                if (agg_it->first == "runtime") {
                    m_is_runtime_requested = true;
                    m_runtime_slot = slot;
                }
                else {
                    // iterate over signal domains.
                    for (auto sig_dom_it = m_signal.begin(); sig_dom_it != m_signal.end(); ++sig_dom_it) {
                        (*domain_size_it) = (*num_sig_it) * sig_dom_it->second.size();
                        m_num_total_signal += (*domain_size_it);
                        // iterate over signals provided.
                        for (auto sig_it = sig_dom_it->second.begin(); sig_it != sig_dom_it->second.end(); ++sig_it) {
                            // if we find the signal to be aggregated, calculate it's offset in the telemetry vector.
                            if (agg_it->first == (*sig_it)) {
                                // signal offset.
                                agg_desc_it->offset = offset;
                                // signal range.
                                agg_desc_it->range = (*num_sig_it);
                                // spatial signal operator.
                                agg_desc_it->spatial_op = agg_it->second.first;
                                // temporal signal operator.
                                agg_desc_it->temporal_op = agg_it->second.second;
                                ++agg_desc_it;
                            }
                            // add the number of signals for this signal type.
                            offset += (*num_sig_it);
                        }
                        ++num_sig_it;
                    }
                }
                ++slot;
            }
            m_entry_telemetry.resize(m_num_total_signal);
            std::fill(m_entry_telemetry.begin(), m_entry_telemetry.end(), -1.0);
        }
        else { //tree level
            m_domain_buffer.push_back(new CircularBuffer<std::vector<double> >(M_NUM_SAMPLE_HISTORY));
            config.get_aggregate(m_aggregate_signal);
            int entry_per_signal = config.num_signal_per_domain(GEOPM_DOMAIN_SIGNAL_NODE);
            m_domain_size.push_back(entry_per_signal);
            m_num_total_signal = entry_per_signal * m_aggregate_signal.size();
            m_entry_telemetry.resize(m_num_total_signal, -1.0);
            m_curr_sample.resize(m_aggregate_signal.size(), 0.0);
            m_agg_stat.resize(m_aggregate_signal.size());
            m_agg_desc.resize(m_aggregate_signal.size());
            auto agg_desc_it = m_agg_desc.begin();
            int offset = 0;
            for (auto agg_it = m_aggregate_signal.begin(); agg_it != m_aggregate_signal.end(); ++agg_it) {
                agg_desc_it->offset = offset;
                agg_desc_it->range = entry_per_signal;
                agg_desc_it->spatial_op = agg_it->second.first;
                agg_desc_it->temporal_op = agg_it->second.second;
                ++agg_desc_it;
                ++agg_it;
                offset += entry_per_signal;
            }
        }
        m_min.resize(m_domain_buffer.size());
        m_max.resize(m_domain_buffer.size());
        m_sum.resize(m_domain_buffer.size());
        m_sum_squares.resize(m_domain_buffer.size());
        m_derivative_last.resize(m_domain_buffer.size());
        for (int i = 0 ; i < m_domain_buffer.size(); ++i) {
            m_min[i].resize(m_domain_size[i], (const double)(DBL_MAX));
            m_max[i].resize(m_domain_size[i], (const double)(-DBL_MAX));
            m_sum[i].resize(m_domain_size[i], 0.0);
            m_sum_squares[i].resize(m_domain_size[i], 0.0);
            m_derivative_last[i].resize(m_domain_size[i], NAN);
        }
    }

    Region::~Region()
    {
        if (m_time_buffer != NULL) {
            delete m_time_buffer;
        }
        for (auto it = m_domain_buffer.rbegin(); it != m_domain_buffer.rend(); ++it) {
            if ((*it) != NULL) {
                delete (*it);
            }
        }
    }

    void Region::entry(void)
    {
        ++m_num_entry;
    }

    int Region::num_entry(void)
    {
        return m_num_entry;
    }

    void Region::insert(const struct geopm_time_s timestamp, std::vector<double> &telemetry, int status)
    {
        if (telemetry.size() != m_num_total_signal) {
            throw Exception("Region::insert(): telemetry not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        
        int stat = status;
        m_time_buffer->insert(timestamp);
        if (status == TELEMETRY_STATUS_EPOCH) {
            stat = TELEMETRY_STATUS_EXIT;
        }
        if (stat == TELEMETRY_STATUS_EXIT && geopm_time_zero(&m_entry_timestamp)) {
            m_last_runtime = geopm_time_diff(&m_entry_timestamp, &timestamp);
            m_agg_runtime += m_last_runtime;
            auto agg_desc_it = m_agg_desc.begin();
            auto agg_stat_it = m_agg_stat.begin();
            int slot = 0;
            for (auto sample_it = m_curr_sample.begin(); sample_it != m_curr_sample.end(); ++sample_it) {
                if (m_is_runtime_requested && m_runtime_slot == slot) {
                    (*sample_it) = m_last_runtime;
                }
                else {
                    (*sample_it) = 0.0;
                    //spatial reduction
                    for (int i = agg_desc_it->offset; i < (agg_desc_it->offset + agg_desc_it->range); ++i) {
                        double min = DBL_MAX;
                        double max = DBL_MIN;
                        switch (agg_desc_it->spatial_op) {
                            case AGGREGATION_OP_SUM:
                            case AGGREGATION_OP_AVG:
                                (*sample_it) += telemetry[i] - m_entry_telemetry[i];
                                break;
                            case AGGREGATION_OP_MIN:
                                if (telemetry[i] - m_entry_telemetry[i] < min) {
                                    min = telemetry[i] - m_entry_telemetry[i];
                                    (*sample_it) = min;
                                }
                                break;
                            case AGGREGATION_OP_MAX:
                                if (telemetry[i] - m_entry_telemetry[i] > max) {
                                    max = telemetry[i] - m_entry_telemetry[i];
                                    (*sample_it) = max;
                                }
                                break;
                            default:
                                throw geopm::Exception("Region::insert(): unknown reduction operation.",
                                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                        }
                    }
                    if (agg_desc_it->spatial_op == AGGREGATION_OP_AVG) {
                        (*sample_it) /= agg_desc_it->range;
                    }
                }
                (*agg_stat_it) += (*sample_it);
                ++agg_desc_it;
                ++agg_stat_it;
                ++slot;
            }
        }
        if (status == TELEMETRY_STATUS_EPOCH) {
            stat = TELEMETRY_STATUS_ENTRY;
        }
        if (stat == TELEMETRY_STATUS_ENTRY) {
            m_entry_timestamp = timestamp;
            m_entry_telemetry = telemetry;
        }
        unsigned domain_idx = 0;
        auto telemetry_it = telemetry.begin(); 
        for (auto domain_size_it = m_domain_size.begin(); domain_size_it != m_domain_size.end(); ++domain_size_it, ++domain_idx) {
            std::vector<double> domain_signal(telemetry_it, telemetry_it + (*domain_size_it));
            update_stats(domain_idx, domain_signal);
            m_domain_buffer[domain_idx]->insert(domain_signal);
            ++domain_idx;
            telemetry_it += (*domain_size_it);
        }
        if (m_derivative_num_fit < M_NUM_SAMPLE_HISTORY) {
            ++m_derivative_num_fit;
        }
    }

    void Region::clear(void)
    {
        m_derivative_num_fit = 0;
        m_time_buffer->clear();
        for (int i = 0; i < m_domain_buffer.size(); ++i) {
            m_domain_buffer[i]->clear();
            std::fill(m_min[i].begin(), m_min[i].end(), DBL_MAX);
            std::fill(m_max[i].begin(), m_max[i].end(), -DBL_MAX);
            std::fill(m_sum[i].begin(), m_sum[i].end(), 0.0);
            std::fill(m_sum_squares[i].begin(), m_sum_squares[i].end(), 0.0);
        }
    }

    void Region::increment_mpi_time(double mpi_increment_amount)
    {
        m_mpi_time += mpi_increment_amount;
    }

    uint64_t Region::identifier(void) const
    {
        return m_identifier;
    }

    void Region::sample_message(std::vector<double> &sample) const
    {
        sample = m_curr_sample;
    }

    void Region::signal(int domain_idx, int signal_idx, std::vector<double> &signal) const
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (signal.size() != m_num_sig_domain[domain_idx]) {
            signal.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        int num_entries = m_num_sig_domain[domain_idx];
        off_t offset = signal_idx * num_entries;
        for (int i = 0 ; i < num_entries; ++i) {
            signal[i] = (m_domain_buffer[domain_idx]->value(m_domain_buffer[domain_idx]->size() - 1))[offset + i];
        }
    }

    void Region::mean(int domain_idx, int signal_idx, std::vector<double> &mean) const
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (mean.size() != m_num_sig_domain[domain_idx]) {
            mean.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        auto it = m_sum[domain_idx].begin() + signal_idx * m_num_sig_domain[domain_idx];
        mean.assign(it, it + m_num_sig_domain[domain_idx]);
    }

    void Region::median(int domain_idx, int signal_idx, std::vector<double> &median) const
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (median.size() != m_num_sig_domain[domain_idx]) {
            median.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        std::vector<double> median_sort(m_domain_buffer[domain_idx]->size());
        for (int i = 0; i < m_num_sig_domain[domain_idx]; ++i) {
            for (int idx = 0; idx < m_domain_buffer[domain_idx]->size(); ++idx) {
                median_sort[idx] = m_domain_buffer[domain_idx]->value(i)[signal_idx * m_num_sig_domain[domain_idx] + i];
            }
            std::sort(median_sort.begin(), median_sort.end());
            median[i] = median_sort[median_sort.size() / 2];
        }
    }

    void Region::std_deviation(int domain_idx, int signal_idx, std::vector<double> &std_deviation) const
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (std_deviation.size() != m_num_sig_domain[domain_idx]) {
            std_deviation.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        double nn = m_num_sig_domain[domain_idx];
        std::vector<double> mm;
         mean(domain_idx, signal_idx, mm);
        for (int i = 0; i < m_num_sig_domain[domain_idx]; ++i) {
            double ss = m_sum_squares[domain_idx][signal_idx * nn + i];
            std_deviation[i] = std::sqrt(ss / nn - mm[i] * mm[i]);
        }
    }

    void Region::min(int domain_idx, int signal_idx, std::vector<double> &min) const
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (min.size() != m_num_sig_domain[domain_idx]) {
            min.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        for (int i = 0; i < m_num_sig_domain[domain_idx]; ++i) {
            min[i] = m_min[domain_idx][signal_idx * m_num_sig_domain[domain_idx] + i];
        }
    }

    void Region::max(int domain_idx, int signal_idx, std::vector<double> &max) const
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (max.size() != m_num_sig_domain[domain_idx]) {
            max.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        for (int i = 0; i < m_num_sig_domain[domain_idx]; ++i) {
            max[i] = m_max[domain_idx][signal_idx * m_num_sig_domain[domain_idx] + i];
        }
    }

    void Region::derivative(int domain_idx, int signal_idx, std::vector<double> &derivative)
    {
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (derivative.size() != m_num_sig_domain[domain_idx]) {
            derivative.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        // Least squares linear regression to approximate the
        // derivative with noisy data.
        for (int i = 0; i < m_num_sig_domain[domain_idx]; ++i) {
            size_t sig_off = signal_idx * m_num_sig_domain[domain_idx] + i;
            double result = m_derivative_last[domain_idx][sig_off];
            if (m_derivative_num_fit >= 2) {
                size_t buf_size = m_time_buffer->size();
                double A = 0.0, B = 0.0, C = 0.0, D = 0.0;
                double E = 1.0 / m_derivative_num_fit;
                const struct geopm_time_s &time_0 = m_time_buffer->value(buf_size - m_derivative_num_fit);
                const double sig_0 = m_domain_buffer[domain_idx]->value(buf_size - m_derivative_num_fit)[sig_off];
                for (size_t buf_off = buf_size - m_derivative_num_fit;
                     buf_off < buf_size; ++buf_off) {
                    const struct geopm_time_s &tt = m_time_buffer->value(buf_off);
                    double time = geopm_time_diff(&time_0, &tt);
                    double sig = m_domain_buffer[domain_idx]->value(buf_off)[sig_off] - sig_0;
                    A += time * sig;
                    B += time;
                    C += sig;
                    D += time * time;
                }
                double ssxx = D - B * B * E;
                double ssxy = A - B * C * E;
                result = ssxy / ssxx;
                m_derivative_last[domain_idx][sig_off] = result;
            }
            derivative[i] = result ? result : NAN;
        }
    }

    void Region::integral(int domain_idx, int signal_idx, double &delta_time, std::vector<double> &integral) const
    {
        throw Exception("Region::integrate_time()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        check_bounds(domain_idx, signal_idx, __FILE__, __LINE__);
        if (integral.size() != m_num_sig_domain[domain_idx]) {
            integral.resize(m_num_sig_domain[domain_idx], 0.0);
        }
        std::fill(integral.begin(), integral.end(), NAN);
    }

    void Region::report(std::ostringstream &string_stream, const std::string &name, int num_rank_per_node) const
    {
        if (!m_level) {
            string_stream << "Region " << name << " (" << m_identifier << "):" << std::endl;
            string_stream << "\truntime (sec): " << m_agg_runtime << std::endl;
            string_stream << "\tmpi-runtime (sec): " << m_mpi_time << std::endl;
            auto signal_it = m_aggregate_signal.begin();
            for (auto it = m_agg_stat.begin(); it != m_agg_stat.end(); ++it) {
                string_stream << "\t" << signal_it->first << ": " << (*it) << std::endl;
                ++signal_it;
            }
            // For epoch, remove two counts: one for startup call and
            // one for shutdown call.  For umarked code just print 0
            // for count. For other regions normalize by number of
            // ranks per node since each rank reports entry
            // (unlike epoch which reports once per node).
            double count = 0.0;
            if (m_identifier == GEOPM_REGION_ID_EPOCH) {
                count = m_num_entry;
            }
            else if (m_identifier != GEOPM_REGION_ID_UNMARKED) {
                count = (double)m_num_entry / num_rank_per_node;
            }
            string_stream << "\tcount: " << count << std::endl;
        }
    }

    // Protected function definitions

    void Region::check_bounds(int domain_idx, int signal_idx, const char *file, int line) const
    {
        if (domain_idx < 0 || domain_idx >= m_domain_buffer.size()) {
            throw geopm::Exception("Region::check_bounds(): the requested domain index is out of bounds.",
                                   GEOPM_ERROR_INVALID, file, line);
        }
        if (signal_idx < 0 || signal_idx >= (m_domain_buffer[domain_idx]->size() / m_num_sig_domain[domain_idx])) {
            throw geopm::Exception("Region::check_bounds(): the requested signal type is invalid.",
                                   GEOPM_ERROR_INVALID, file, line);
        }
    }

    void Region::update_stats(int domain_idx, const std::vector<double> &signal)
    {
        bool is_full = m_domain_buffer[domain_idx]->size() == m_domain_buffer[domain_idx]->capacity();
        for (int i = 0; i < m_domain_size[domain_idx]; ++i) {
            // CALCULATE THE MIN
            if (signal[i] < m_min[domain_idx][i]) {
                m_min[domain_idx][i] = signal[i];
            }
            else if (is_full && m_min[domain_idx][i] == m_domain_buffer[domain_idx]->value(0)[i]) {
                // We are about to throw out the current min
                // Find the new one
                m_min[domain_idx][i] = signal[i];
                for (int entry = 1; entry < m_domain_buffer[domain_idx]->size(); ++entry) {
                    if (m_domain_buffer[domain_idx]->value(entry)[i] < m_min[domain_idx][i]) {
                        m_min[domain_idx][i] = m_domain_buffer[domain_idx]->value(entry)[i];
                    }
                }
            }

            // CALCULATE THE MAX
            if (signal[i] > m_max[domain_idx][i]) {
                m_max[domain_idx][i] = signal[i];
            }
            else if (is_full && m_max[domain_idx][i] == m_domain_buffer[domain_idx]->value(0)[i]) {
                // We are about to throw out the current max
                // Find the new one
                m_max[domain_idx][i] = signal[i];
                for (int entry = 1; entry < m_domain_buffer[domain_idx]->size(); ++entry) {
                    if (m_domain_buffer[domain_idx]->value(entry)[i] > m_max[domain_idx][i]) {
                        m_max[domain_idx][i] = m_domain_buffer[domain_idx]->value(entry)[i];
                    }
                }
            }

            // CALCULATE SUM AND SUM OF SQUARES
            // sum the values
            m_sum[domain_idx][i] += signal[i];
            // sum the square of the values
            m_sum_squares[domain_idx][i] += signal[i] * signal[i];
            if (is_full) {
                // We need to subtract the value of the signal we are removing
                m_sum[domain_idx][i] -= m_domain_buffer[domain_idx]->value(0)[i];
                m_sum_squares[domain_idx][i] -= m_domain_buffer[domain_idx]->value(0)[i] * m_domain_buffer[domain_idx]->value(0)[i];
            }
        }
    }
}
