/*
 * Copyright (c) 2015, 2016, Intel Corporation
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
#include <fstream>

#include "Region.hpp"

namespace geopm
{
    Region::Region(uint64_t identifier, int hint, int num_domain, int level)
        : m_identifier(identifier)
        , m_hint(hint)
        , m_num_domain(num_domain)
        , m_level(level)
        , m_num_signal(m_level == 0 ? (int)GEOPM_NUM_TELEMETRY_TYPE : (int)GEOPM_NUM_SAMPLE_TYPE)
        , m_signal_matrix(m_num_signal * m_num_domain)
        , m_entry_telemetry(m_num_domain)
        , m_domain_sample(m_num_domain)
        , m_is_dirty_domain_sample(m_num_domain)
        , m_curr_sample({m_identifier, {0.0, 0.0, 0.0}})
        , m_domain_buffer(M_NUM_SAMPLE_HISTORY)
        , m_time_buffer(M_NUM_SAMPLE_HISTORY)
        , m_valid_entries(m_num_signal * m_num_domain)
        , m_min(m_num_signal * m_num_domain)
        , m_max(m_num_signal * m_num_domain)
        , m_sum(m_num_signal * m_num_domain)
        , m_sum_squares(m_num_signal * m_num_domain)
        , m_agg_stats({m_identifier, {0.0, 0.0, 0.0}})

    {
        std::fill(m_is_dirty_domain_sample.begin(), m_is_dirty_domain_sample.end(), true);
        std::fill(m_min.begin(), m_min.end(), DBL_MAX);
        std::fill(m_max.begin(), m_max.end(), DBL_MIN);
        std::fill(m_sum.begin(), m_sum.end(), 0.0);
        std::fill(m_sum_squares.begin(), m_sum_squares.end(), 0.0);
        std::fill(m_valid_entries.begin(), m_valid_entries.end(), 0);
    }

    Region::~Region()
    {

    }

    void Region::insert(std::vector<struct geopm_telemetry_message_s> &telemetry)
    {
        if (telemetry.size()!= m_num_domain) {
            throw Exception("Region::insert(): telemetry not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_time_buffer.insert(telemetry[0].timestamp);
        unsigned domain_idx = 0;
        size_t offset = 0;
        for (auto it = telemetry.begin(); it != telemetry.end(); ++it) {
            if ((*it).signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 0.0 && //We are entering the region
                (m_domain_buffer.size() == 0 || // the buffer is empty
                 // We have already entered and have not recieved an updated progress signal since then
                 m_domain_buffer.value(m_domain_buffer.size() - 1)[offset + GEOPM_TELEMETRY_TYPE_PROGRESS] != 0.0)) {
                m_entry_telemetry[domain_idx] = (*it);
            }
            if ((*it).signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 1.0) { //We are exiting the region
                m_is_dirty_domain_sample[domain_idx] = false;
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                    geopm_time_diff(&(m_entry_telemetry[domain_idx].timestamp), &((*it).timestamp));
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_ENERGY] =
                    ((*it).signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] +
                     (*it).signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY]) -
                    (m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] +
                     m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY]);
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY] =
                    ((*it).signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE] -
                     m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE]) /
                    m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME];
                m_agg_stats.signal[GEOPM_SAMPLE_TYPE_RUNTIME] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME];
                m_agg_stats.signal[GEOPM_SAMPLE_TYPE_ENERGY] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_ENERGY];
            }
            memcpy(m_signal_matrix.data() + offset, (*it).signal, m_num_signal * sizeof(double));

            std::vector<double> entry_oldest;
            bool is_oldest_valid = false;
            bool is_signal_valid = (*it).signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
            if (m_domain_buffer.size()) {
                entry_oldest = m_domain_buffer.value(0);
                is_oldest_valid = entry_oldest[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                if (!is_oldest_valid ||
                    m_domain_buffer.size() < m_domain_buffer.capacity()) {
                    if (is_signal_valid) {
                        if (m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS] < m_domain_buffer.capacity()) {
                            ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
                            ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
                        }
                    }
                }
                else {
                    if (!is_signal_valid) {
                        --m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
                        --m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
                    }
                }
            }
            else if (is_signal_valid) {
                ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
                ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
            }

            int num_entries = fmin(m_domain_buffer.size() + 1, m_domain_buffer.capacity());
            std::fill(m_valid_entries.begin() + offset, m_valid_entries.begin() + offset + GEOPM_TELEMETRY_TYPE_LLC_VICTIMS, num_entries);

            // Update Min
            for (int i = 0; i < m_num_signal; ++i) {
                // if i references a valid sample
                bool is_runtime_invalid = (i == GEOPM_TELEMETRY_TYPE_RUNTIME ||
                                           i == GEOPM_TELEMETRY_TYPE_PROGRESS) &&
                                           !is_signal_valid;
                // Calculate the min
                if ((*it).signal[i] < m_min[offset + i] && !is_runtime_invalid) {
                    m_min[offset + i] = (*it).signal[i];
                }
                else if (m_domain_buffer.size() == m_domain_buffer.capacity() && m_min[offset + i] == entry_oldest[offset + i]) {
                    //We are about to throw out the current min
                    //Find the new one
                    m_min[offset +i] = is_runtime_invalid ? DBL_MAX : (*it).signal[i];
                    for (int entry = 1; entry < m_domain_buffer.size(); ++entry) {
                        bool is_old_value_valid = (i != GEOPM_TELEMETRY_TYPE_RUNTIME &&
                                                   i != GEOPM_TELEMETRY_TYPE_PROGRESS) ||
                                                   m_domain_buffer.value(entry)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                        if (m_domain_buffer.value(entry)[offset + i] < m_min[offset + i] && is_old_value_valid) {
                            m_min[offset +i] = m_domain_buffer.value(entry)[offset + i];
                        }
                    }
                }
                // Calculate the max
                if ((*it).signal[i] > m_max[offset + i] && !is_runtime_invalid) {
                    m_max[offset + i] = (*it).signal[i];
                }
                else if (m_domain_buffer.size() == m_domain_buffer.capacity() && m_max[offset + i] == entry_oldest[offset + i]) {
                    //We are about to throw out the current max
                    //Find the new one
                    m_max[offset +i] = is_runtime_invalid ? DBL_MIN : (*it).signal[i];
                    for (int entry = 1; entry < m_domain_buffer.size(); ++entry) {
                        bool is_old_value_valid = (i != GEOPM_TELEMETRY_TYPE_RUNTIME &&
                                                   i != GEOPM_TELEMETRY_TYPE_PROGRESS) ||
                                                   m_domain_buffer.value(entry)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                        if (m_domain_buffer.value(entry)[offset + i] > m_max[offset + i] && is_old_value_valid) {
                            m_max[offset +i] = m_domain_buffer.value(entry)[offset + i];
                        }
                    }
                }
                if (m_domain_buffer.size() < m_domain_buffer.capacity()) {
                    if (!is_runtime_invalid) {
                        // sum the values
                        m_sum[offset + i] += (*it).signal[i];
                        // sum the square of the values
                        m_sum_squares[offset + i] += pow((*it).signal[i], 2);
                    }
                }
                else {
                    // We need to throw away the value of the signal we are removing
                    if (is_oldest_valid) {
                        m_sum[offset + i] -= entry_oldest[offset + i];
                        m_sum_squares[offset + i] -= pow(entry_oldest[offset + i], 2);
                    }

                    if (!is_runtime_invalid) {
                        // sum the values
                        m_sum[offset + i] += (*it).signal[i];
                        // sum the square of the values
                        m_sum_squares[offset + i] += pow((*it).signal[i], 2);
                    }
                }
            }

            offset += m_num_signal;
            ++domain_idx;
        }

        m_domain_buffer.insert(m_signal_matrix);

        // Make sure every domain has completed the region
        for (domain_idx = 0; !m_is_dirty_domain_sample[domain_idx] && domain_idx != m_num_domain; ++domain_idx);
        if (domain_idx == m_num_domain) {
            std::fill(m_curr_sample.signal, m_curr_sample.signal + m_num_signal, 0.0);
            for (domain_idx = 0; domain_idx != m_num_domain; ++domain_idx) {
                m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                    m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] > m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] ?
                    m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] : m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
                m_curr_sample.signal[GEOPM_SAMPLE_TYPE_ENERGY] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_ENERGY];
                m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY] += m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY];
                m_is_dirty_domain_sample[domain_idx] = true;
            }
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY] /= m_num_domain;
        }
    }


    void Region::insert(const std::vector<struct geopm_sample_message_s> &sample)
    {
        std::fill(m_curr_sample.signal, m_curr_sample.signal + m_num_signal, 0.0);
        size_t offset = 0;
        for (auto it = sample.begin(); it != sample.end(); ++it) {
            memcpy(m_signal_matrix.data() + offset, (*it).signal, m_num_signal * sizeof(double));
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                (*it).signal[GEOPM_SAMPLE_TYPE_RUNTIME] > m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] ?
                (*it).signal[GEOPM_SAMPLE_TYPE_RUNTIME] : m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_ENERGY] += (*it).signal[GEOPM_SAMPLE_TYPE_ENERGY];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY] += (*it).signal[GEOPM_SAMPLE_TYPE_FREQUENCY];

            int num_entries = fmin(m_domain_buffer.size() + 1, m_domain_buffer.capacity());
            std::fill(m_valid_entries.begin(), m_valid_entries.end(), num_entries);

            for (int i = 0; i < m_num_signal; ++i) {
                std::vector<double> entry_oldest;
                if (m_domain_buffer.size()) {
                    entry_oldest = m_domain_buffer.value(0);
                    // calculate the min
                    if ((*it).signal[i] < m_min[offset + i]) {
                        m_min[offset + i] = (*it).signal[i];
                    }
                    else if (m_domain_buffer.size() == m_domain_buffer.capacity() &&
                             m_min[offset + i] == entry_oldest[offset + i]) {
                        //We are about to throw out the current min
                        //Find the new one
                        m_min[offset +i] = (*it).signal[i];
                        for (int entry = 1; entry < m_domain_buffer.size(); ++entry) {
                            if (m_domain_buffer.value(entry)[offset + i] < m_min[offset + i]) {
                                m_min[offset +i] = m_domain_buffer.value(entry)[offset + i];
                            }
                        }
                    }
                    // calculate the max
                    if ((*it).signal[i] > m_max[offset + i]) {
                        m_max[offset + i] = (*it).signal[i];
                    }
                    else if (m_domain_buffer.size() == m_domain_buffer.capacity() &&
                             m_max[offset + i] == entry_oldest[offset + i]) {
                        //We are about to throw out the current max
                        //Find the new one
                        m_max[offset +i] = (*it).signal[i];
                        for (int entry = 1; entry < m_domain_buffer.size(); ++entry) {
                            if (m_domain_buffer.value(entry)[offset + i] > m_max[offset + i]) {
                                m_max[offset +i] = m_domain_buffer.value(entry)[offset + i];
                            }
                        }
                    }
                    if (m_domain_buffer.size() < m_domain_buffer.capacity()) {
                        // sum the values
                        m_sum[offset + i] += (*it).signal[i];
                        // sum the square of the values
                        m_sum_squares[offset + i] += pow((*it).signal[i], 2);
                    }
                    else {
                        // We need to throw away the value of the signal we are removing
                        // sum the values
                        m_sum[offset + i] += ((*it).signal[i] - entry_oldest[offset + i]);
                        // sum the square of the values
                        m_sum_squares[offset + i] += (pow((*it).signal[i], 2) - pow(entry_oldest[offset + i], 2));
                    }
                }
                else {
                    // calculate the min
                    if ((*it).signal[i] < m_min[offset + i]) {
                        m_min[offset + i] = (*it).signal[i];
                    }
                    // calculate the max
                    if ((*it).signal[i] > m_max[offset + i]) {
                        m_max[offset + i] = (*it).signal[i];
                    }
                    // sum the values
                    m_sum[offset + i] += (*it).signal[i];
                    // sum the square of the values
                    m_sum_squares[offset + i] += pow((*it).signal[i], 2);
                }
            }
            offset += m_num_signal;
        }
        m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY] /= m_num_domain;

        m_domain_buffer.insert(m_signal_matrix);
    }

    uint64_t Region::identifier(void) const
    {
        return m_identifier;
    }

    int Region::hint(void) const
    {
        return m_hint;
    }

    void Region::sample_message(struct geopm_sample_message_s &sample)
    {
        sample = m_curr_sample;
    }

    double Region::signal(int domain_idx, int signal_type)
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        double result = NAN;
        bool is_known_valid = true;
        if (!m_level) {
            is_known_valid = signal_type != GEOPM_TELEMETRY_TYPE_PROGRESS && signal_type != GEOPM_TELEMETRY_TYPE_RUNTIME;
        }
        for (int i = 0; i < m_domain_buffer.size(); ++i) {
            const std::vector<double> &signal_matrix = m_domain_buffer.value(i);
            if (is_known_valid ||
                signal_matrix[domain_idx * m_num_signal + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1) {
                result = signal_matrix[domain_idx * m_num_signal + signal_type];
            }
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
        for (int i = 0; i < m_domain_buffer.size(); ++i) {
            if (is_known_valid ||
                m_domain_buffer.value(i)[m_num_signal * domain_idx + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0) {
                    median_sort[idx++] = m_domain_buffer.value(i)[m_num_signal * domain_idx + signal_type];
            }
        }
        std::sort(median_sort.begin(), median_sort.begin() + num_sample(domain_idx, signal_type));
        return median_sort[num_sample(domain_idx, signal_type) / 2];
    }

    double Region::std_deviation(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return sqrt((m_sum_squares[domain_idx * m_num_signal + signal_type] /
                num_sample(domain_idx, signal_type)) -
                pow(mean(domain_idx, signal_type), 2));
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

    double Region::derivative(int domain_idx, int signal_type) const
    {
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        if (m_level) {
            throw Exception("Region::derivative(): Not implemented for non-leaf", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
        double result = NAN;
        if (m_domain_buffer.size() >= 2) {
            const std::vector<double> &signal_matrix_0 = m_domain_buffer.value(m_domain_buffer.size() - 2);
            const std::vector<double> &signal_matrix_1 = m_domain_buffer.value(m_domain_buffer.size() - 1);
            double delta_signal = signal_matrix_1[domain_idx * m_num_signal + signal_type] -
                                  signal_matrix_0[domain_idx * m_num_signal + signal_type];
            const struct geopm_time_s &time_0 = m_time_buffer.value(m_time_buffer.size() - 2);
            const struct geopm_time_s &time_1 = m_time_buffer.value(m_time_buffer.size() - 1);
            double delta_time = geopm_time_diff(&time_0, &time_1);
            result = delta_signal / delta_time;
        }
        return result;
    }

    double Region::integral(int domain_idx, int signal_type, double &delta_time, double &integral) const
    {
        throw Exception("Region::integrate_time()", GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        check_bounds(domain_idx, signal_type, __FILE__, __LINE__);
        return 0.0;
    }

    void Region::check_bounds(int domain_idx, int signal_type, const char *file, int line) const
    {
        if (domain_idx < 0 || domain_idx > (int)m_num_domain) {
            throw geopm::Exception("Region::check_bounds(): the requested domain index is out of bounds. called from geopm/"
                                   + std::string(file) + ":" + std::to_string(line),
                                   GEOPM_ERROR_INVALID);
        }
        if (signal_type < 0 || signal_type > m_num_signal) {
            throw geopm::Exception("Region::check_bounds(): the requested signal type is invalid. called from geopm/"
                                   + std::string(file) + ":" + std::to_string(line),
                                   GEOPM_ERROR_INVALID);
        }
    }

    void Region::report(std::ofstream &file_stream, const std::string &name) const
    {
        file_stream << "Region " + name + ":" << std::endl;
        file_stream << "\truntime: " << m_agg_stats.signal[GEOPM_SAMPLE_TYPE_RUNTIME] << std::endl;
    }
}
