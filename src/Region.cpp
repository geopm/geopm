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
    Region::Region(uint64_t identifier, int num_domain, int level, IProfileThreadTable *tprof_table)
        : m_identifier(identifier)
        , m_num_domain(num_domain)
        , m_level(level)
        , m_num_signal(m_level == 0 ? (int)GEOPM_NUM_TELEMETRY_TYPE : (int)GEOPM_NUM_SAMPLE_TYPE)
        , m_signal_matrix(m_num_signal * m_num_domain)
        , m_entry_telemetry(m_num_domain, {GEOPM_REGION_ID_UNDEFINED, {{0, 0}}, {0}})
        , m_curr_sample({m_identifier, {0.0, 0.0, 0.0, 0.0}})
        , m_domain_sample(m_num_domain, m_curr_sample)
        , m_domain_buffer(NULL)
        , m_time_buffer(NULL)
        , m_valid_entries(m_num_signal * m_num_domain, 0)
        , m_min(m_num_signal * m_num_domain, DBL_MAX)
        , m_max(m_num_signal * m_num_domain, -DBL_MAX)
        , m_sum(m_num_signal * m_num_domain, 0.0)
        , m_sum_squares(m_num_signal * m_num_domain, 0.0)
        , m_derivative_last(m_num_signal * m_num_domain, NAN)
        , m_agg_stats({m_identifier, {0.0, 0.0, 0.0, 0.0}})
        , m_num_entry(0)
        , m_is_entered(m_num_domain, false)
        , m_derivative_num_fit(0)
        , m_mpi_time(0.0)
        , m_tprof_table(tprof_table)
    {
        m_domain_buffer = new CircularBuffer<std::vector<double> >(M_NUM_SAMPLE_HISTORY);
        m_time_buffer = new CircularBuffer<struct geopm_time_s>(M_NUM_SAMPLE_HISTORY);
    }

    Region::~Region()
    {
        delete m_time_buffer;
        delete m_domain_buffer;

    }

    void Region::entry(void)
    {
        ++m_num_entry;
    }

    int Region::num_entry(void)
    {
        return m_num_entry;
    }

    void Region::insert(std::vector<struct geopm_telemetry_message_s> &telemetry)
    {
        if (telemetry.size()!= m_num_domain) {
            throw Exception("Region::insert(): telemetry not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_time_buffer->insert(telemetry[0].timestamp);
        unsigned domain_idx = 0;
        for (auto it = telemetry.begin(); it != telemetry.end(); ++it, ++domain_idx) {
#ifdef GEOPM_DEBUG
            if (geopm_time_diff(&((*it).timestamp), &(telemetry[0].timestamp)) != 0.0) {
                throw Exception("Region::insert(): input telemetry vector has non-uniform timestamp values", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if ((*it).region_id != m_identifier) {
                std::ostringstream ex_str;
                ex_str << "Region::insert(): input telemetry vector wrong region id, expecting: "  << m_identifier  << ", recieved: " << (*it).region_id;
                throw Exception(ex_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
#endif
            update_domain_sample(*it, domain_idx);
            update_signal_matrix((*it).signal, domain_idx);
            update_valid_entries(*it, domain_idx);
            update_stats((*it).signal, domain_idx);
        }
        m_domain_buffer->insert(m_signal_matrix);
        // If all ranks have exited the region update current sample
        for (domain_idx = 0;
             domain_idx != m_num_domain &&
             telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 1.0 &&
             telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
             ++domain_idx);
        if (domain_idx == m_num_domain) {
            // All domains have completed so do update
            update_curr_sample();
            /// @fixme Commenting out the following line. It restarts samples after
            /// leaving a region, which is the athematically correct thing to do.
            /// However we are observing a great deal of run-to-run variance when this
            /// line is enabled. Without this line in we are wrapping all energy usage
            /// in between instances of a region into a single sample.
            //m_derivative_num_fit = 0;
        }
        else if (m_derivative_num_fit < M_NUM_SAMPLE_HISTORY) {
            ++m_derivative_num_fit;
        }
    }

    void Region::insert(const std::vector<struct geopm_sample_message_s> &sample)
    {
        if (sample.size() < m_num_domain) {
            throw Exception("Region::insert(): input sample vector too small",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy(sample.begin(), sample.begin() + m_num_domain, m_domain_sample.begin());
        update_curr_sample();
        // Calculate the number of entries *after* we insert the new data: size() + 1 or capacity
        int num_entries = m_domain_buffer->size() + 1 < m_domain_buffer->capacity() ?
                          m_domain_buffer->size() + 1 : m_domain_buffer->capacity();
        // This insert is called above leaf level, so all entries are valid
        std::fill(m_valid_entries.begin(), m_valid_entries.end(), num_entries);

        auto it = sample.begin();
        for (size_t domain_idx = 0; domain_idx != m_num_domain; ++domain_idx, ++it) {
            update_signal_matrix((*it).signal, domain_idx);
            update_stats((*it).signal, domain_idx);
        }
        m_domain_buffer->insert(m_signal_matrix);
    }

    void Region::clear(void)
    {
        m_derivative_num_fit = 0;
        m_time_buffer->clear();
        m_domain_buffer->clear();
        std::fill(m_min.begin(), m_min.end(), DBL_MAX);
        std::fill(m_max.begin(), m_max.end(), -DBL_MAX);
        std::fill(m_sum.begin(), m_sum.end(), 0.0);
        std::fill(m_sum_squares.begin(), m_sum_squares.end(), 0.0);
        std::fill(m_valid_entries.begin(), m_valid_entries.end(), 0);
    }

    void Region::increment_mpi_time(double mpi_increment_amount)
    {
        m_mpi_time += mpi_increment_amount;
    }

    uint64_t Region::identifier(void) const
    {
        return m_identifier;
    }

    void Region::sample_message(struct geopm_sample_message_s &sample)
    {
        sample = m_curr_sample;
    }

    double Region::signal(int domain_idx, int signal_type)
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        double result = NAN;
        if (!m_level &&
            (signal_type == GEOPM_TELEMETRY_TYPE_PROGRESS ||
             signal_type == GEOPM_TELEMETRY_TYPE_RUNTIME)) {
            for (int i = 0; i < m_domain_buffer->size(); ++i) {
                if (domain_buffer_value(i, domain_idx, GEOPM_TELEMETRY_TYPE_RUNTIME) != -1) {
                    result = domain_buffer_value(i, domain_idx, signal_type);
                }
            }
        }
        else {
            result = domain_buffer_value(-1, domain_idx, signal_type);
        }
        return result;
    }

    int Region::num_sample(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return m_valid_entries[domain_idx * m_num_signal + signal_type];
    }

    double Region::mean(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return  m_sum[domain_idx * m_num_signal + signal_type] /
                num_sample(domain_idx, signal_type);
    }

    double Region::median(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        std::vector<double> median_sort(num_sample(domain_idx, signal_type));
        int idx = 0;
        bool is_known_valid = true;
        if (!m_level) {
            is_known_valid = signal_type != GEOPM_TELEMETRY_TYPE_PROGRESS && signal_type != GEOPM_TELEMETRY_TYPE_RUNTIME;
        }
        for (int i = 0; i < m_domain_buffer->size(); ++i) {
            if (is_known_valid ||
                m_domain_buffer->value(i)[m_num_signal * domain_idx + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0) {
                median_sort[idx++] = m_domain_buffer->value(i)[m_num_signal * domain_idx + signal_type];
            }
        }
        std::sort(median_sort.begin(), median_sort.begin() + num_sample(domain_idx, signal_type));
        return median_sort[num_sample(domain_idx, signal_type) / 2];
    }

    double Region::std_deviation(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        double ss = m_sum_squares[domain_idx * m_num_signal + signal_type];
        double nn = num_sample(domain_idx, signal_type);
        double mm = mean(domain_idx, signal_type);
        return std::sqrt(ss / nn - mm * mm);
    }

    double Region::min(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return m_min[domain_idx * m_num_signal + signal_type];
    }

    double Region::max(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return m_max[domain_idx * m_num_signal + signal_type];
    }

    double Region::derivative(int domain_idx, int signal_type)
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        if (m_level) {
            throw Exception("Region::derivative(): Not implemented for non-leaf",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        // Least squares linear regression to approximate the
        // derivative with noisy data.
        size_t sig_off = domain_idx * m_num_signal + signal_type;
        double result = m_derivative_last[sig_off];
        if (m_derivative_num_fit >= 2) {
            size_t buf_size = m_time_buffer->size();
            double A = 0.0, B = 0.0, C = 0.0, D = 0.0;
            double E = 1.0 / m_derivative_num_fit;
            const struct geopm_time_s &time_0 = m_time_buffer->value(buf_size - m_derivative_num_fit);
            const double sig_0 = m_domain_buffer->value(buf_size - m_derivative_num_fit)[sig_off];
            for (size_t buf_off = buf_size - m_derivative_num_fit;
                 buf_off < buf_size; ++buf_off) {
                const struct geopm_time_s &tt = m_time_buffer->value(buf_off);
                double time = geopm_time_diff(&time_0, &tt);
                double sig = m_domain_buffer->value(buf_off)[sig_off] - sig_0;
                A += time * sig;
                B += time;
                C += sig;
                D += time * time;
            }
            double ssxx = D - B * B * E;
            double ssxy = A - B * C * E;
            result = ssxy / ssxx;
            m_derivative_last[sig_off] = result;
        }
        return result ? result : NAN;
    }

    double Region::integral(int domain_idx, int signal_type, double &delta_time, double &integral) const
    {
        throw Exception("Region::integrate_time()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return 0.0;
    }

    void Region::report(std::ostringstream &string_stream, const std::string &name, int num_rank_per_node) const
    {
        string_stream << "Region " << name << " (" << m_identifier << "):" << std::endl;
        string_stream << "\truntime (sec): " << m_agg_stats.signal[GEOPM_SAMPLE_TYPE_RUNTIME] << std::endl;
        string_stream << "\tenergy (joules): " << m_agg_stats.signal[GEOPM_SAMPLE_TYPE_ENERGY] << std::endl;
        string_stream << "\tfrequency (%): " << (m_agg_stats.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM] ? 100 *
                                               m_agg_stats.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER] /
                                               m_agg_stats.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM] :
                                               0.0) << std::endl;
        string_stream << "\tmpi-runtime (sec): " << m_mpi_time << std::endl;
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

    void Region::thread_progress(std::vector<double> &progress)
    {
        if (m_tprof_table) {
            progress.resize(m_tprof_table->num_cpu());
            m_tprof_table->dump(progress);
        }
    }

    // Protected function definitions

    void Region::check_bounds(int domain_idx, int signal_type, const char *file, int line) const
    {
        if (domain_idx < 0 || domain_idx > (int)m_num_domain) {
            throw geopm::Exception("Region::check_bounds(): the requested domain index is out of bounds.",
                                   GEOPM_ERROR_INVALID, file, line);
        }
        if (signal_type < 0 || signal_type > m_num_signal) {
            throw geopm::Exception("Region::check_bounds(): the requested signal type is invalid.",
                                   GEOPM_ERROR_INVALID, file, line);
        }
    }

    double Region::domain_buffer_value(int buffer_idx, int domain_idx, int signal_type)
    {
        double result = NAN;
#ifdef GEOPM_DEBUG
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
#endif
        // If buffer index is negative then wrap around
        if (buffer_idx < 0) {
            buffer_idx += m_domain_buffer->size();
        }
        if (buffer_idx >= 0 && buffer_idx < m_domain_buffer->size()) {
            result = m_domain_buffer->value(buffer_idx)[domain_idx * m_num_signal + signal_type];
        }
        return result;
    }

    bool Region::is_telemetry_entry(const struct geopm_telemetry_message_s &telemetry, int domain_idx)
    {
        bool result = telemetry.signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 0.0 && // We are entering the region
                      telemetry.signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0 && // The sample is valid
                      !m_is_entered[domain_idx]; // We have not already entered region
        if (result) {
            m_is_entered[domain_idx] = true;
        }
        return result;
    }

    bool Region::is_telemetry_exit(const struct geopm_telemetry_message_s &telemetry, int domain_idx)
    {
        bool result = telemetry.signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 1.0 && // We are exiting the region
                      telemetry.signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0 && // The sample is valid
                      domain_buffer_value(-1, domain_idx, GEOPM_TELEMETRY_TYPE_PROGRESS) != 1.0; // We have not already exited region
        if (result) {
            m_is_entered[domain_idx] = false;
        }
        return result;
    }

    void Region::update_domain_sample(const struct geopm_telemetry_message_s &telemetry, int domain_idx)
    {
        if (is_telemetry_entry(telemetry, domain_idx) ) {
            m_entry_telemetry[domain_idx] = telemetry;
        }
        else if (m_entry_telemetry[domain_idx].region_id != GEOPM_REGION_ID_UNDEFINED &&
                 is_telemetry_exit(telemetry, domain_idx)) {
            m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                geopm_time_diff(&(m_entry_telemetry[domain_idx].timestamp), &(telemetry.timestamp));
            m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_ENERGY] =
                (telemetry.signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] +
                 telemetry.signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY]) -
                (m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] +
                 m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY]);
            m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER] +=
                telemetry.signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE] -
                m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE];
            m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM] +=
                telemetry.signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF] -
                m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF];
            m_entry_telemetry[domain_idx].region_id = GEOPM_REGION_ID_UNDEFINED;
        }
    }

    void Region::update_signal_matrix(const double *signal, int domain_idx)
    {
        memcpy(m_signal_matrix.data() + domain_idx * m_num_signal, signal, m_num_signal * sizeof(double));
    }

    void Region::update_valid_entries(const struct geopm_telemetry_message_s &telemetry, int domain_idx)
    {
        int offset = domain_idx * m_num_signal;
        // Calculate the number of entries *after* we insert the new data: size() + 1 or capacity
        int num_entries = m_domain_buffer->size() + 1 < m_domain_buffer->capacity() ?
                          m_domain_buffer->size() + 1 : m_domain_buffer->capacity();
        // Fill in the number of valid entries for other signals which are always valid
        std::fill(m_valid_entries.begin() + offset, m_valid_entries.begin() + offset + GEOPM_TELEMETRY_TYPE_PROGRESS, num_entries);

        // Account for invalid progress or runtime being inserted or dropped off the end of the buffer
        bool is_oldest_valid = m_domain_buffer->size() &&
                               m_domain_buffer->value(0)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
        bool is_signal_valid = telemetry.signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
        bool is_full = m_domain_buffer->size() == m_domain_buffer->capacity();

        if ((is_full && !is_oldest_valid && is_signal_valid) ||
            (!is_full && is_signal_valid)) {
            ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
            ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
        }
        else if (is_full && is_oldest_valid && !is_signal_valid) {
            --m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
            --m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
        }
    }

    void Region::update_stats(const double *signal, int domain_idx)
    {
        int offset = domain_idx * m_num_signal;
        bool is_full = m_domain_buffer->size() == m_domain_buffer->capacity();
        for (int i = 0; i < m_num_signal; ++i) {
            bool is_signal_valid = m_level ? true : signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;

            // CALCULATE THE MIN
            if (is_signal_valid && signal[i] < m_min[offset + i]) {
                m_min[offset + i] = signal[i];
            }
            else if (is_full && m_min[offset + i] == m_domain_buffer->value(0)[offset + i]) {
                // We are about to throw out the current min
                // Find the new one
                m_min[offset + i] = is_signal_valid ? signal[i] : DBL_MAX;
                for (int entry = 1; entry < m_domain_buffer->size(); ++entry) {
                    bool is_old_value_valid = m_level ? true : m_domain_buffer->value(entry)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                    if (is_old_value_valid &&
                        m_domain_buffer->value(entry)[offset + i] < m_min[offset + i]) {
                        m_min[offset + i] = m_domain_buffer->value(entry)[offset + i];
                    }
                }
            }

            // CALCULATE THE MAX
            if (is_signal_valid && signal[i] > m_max[offset + i]) {
                m_max[offset + i] = signal[i];
            }
            else if (is_full && m_max[offset + i] == m_domain_buffer->value(0)[offset + i]) {
                // We are about to throw out the current max
                // Find the new one
                m_max[offset + i] = is_signal_valid ? signal[i] : -DBL_MAX;
                for (int entry = 1; entry < m_domain_buffer->size(); ++entry) {
                    bool is_old_value_valid = m_level ? true : m_domain_buffer->value(entry)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                    if (is_old_value_valid &&
                        m_domain_buffer->value(entry)[offset + i] > m_max[offset + i]) {
                        m_max[offset +i] = m_domain_buffer->value(entry)[offset + i];
                    }
                }
            }

            // CALCULATE SUM AND SUM OF SQUARES
            bool is_oldest_valid = m_level || (m_domain_buffer->size() &&
                                               m_domain_buffer->value(0)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0);

            if (is_signal_valid) {
                // sum the values
                m_sum[offset + i] += signal[i];
                // sum the square of the values
                m_sum_squares[offset + i] += signal[i] * signal[i];
            }
            if (is_full && is_oldest_valid) {
                // We need to subtract the value of the signal we are removing
                m_sum[offset + i] -= m_domain_buffer->value(0)[offset + i];
                m_sum_squares[offset + i] -= m_domain_buffer->value(0)[offset + i] * m_domain_buffer->value(0)[offset + i];
            }
        }
    }

    void Region::update_curr_sample(void)
    {
        std::fill(m_curr_sample.signal, m_curr_sample.signal + GEOPM_NUM_SAMPLE_TYPE, 0.0);
        for (unsigned domain_idx = 0; domain_idx != m_num_domain; ++domain_idx) {
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] > m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] ?
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] : m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_ENERGY] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_ENERGY];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM];
        }
        m_agg_stats.signal[GEOPM_SAMPLE_TYPE_RUNTIME] += m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
        m_agg_stats.signal[GEOPM_SAMPLE_TYPE_ENERGY] += m_curr_sample.signal[GEOPM_SAMPLE_TYPE_ENERGY];
        m_agg_stats.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER] += m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_NUMER];
        m_agg_stats.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM] += m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY_DENOM];
    }

}
