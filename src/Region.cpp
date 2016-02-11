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

#include "Region.hpp"

namespace geopm
{
    Region::Region(uint64_t identifier, int hint, int num_domain, int level)
        : m_identifier(identifier)
        , m_hint(hint)
        , m_num_domain(num_domain)
        , m_level(level)
        , m_signal_matrix(m_num_domain * (m_level == 0 ? (int)GEOPM_NUM_TELEMETRY_TYPE : (int)GEOPM_NUM_SAMPLE_TYPE))
        , m_entry_telemetry(m_num_domain)
        , m_domain_sample(m_num_domain)
        , m_is_dirty_domain_sample(m_num_domain)
        , m_curr_sample({m_identifier, {0.0, 0.0, 0.0}})
        , m_domain_buffer(M_NUM_SAMPLE_HISTORY)
        , m_time_buffer(M_NUM_SAMPLE_HISTORY)
        , m_min(GEOPM_NUM_SAMPLE_TYPE * m_num_domain)
        , m_max(GEOPM_NUM_SAMPLE_TYPE * m_num_domain)
        , m_sum(GEOPM_NUM_SAMPLE_TYPE * m_num_domain)
        , m_sum_squares(GEOPM_NUM_SAMPLE_TYPE * m_num_domain)

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

    void Region::insert(std::stack<struct geopm_telemetry_message_s> &telemetry_stack)
    {
        if (telemetry_stack.size()!= m_num_domain) {
            throw Exception("Region::insert(): telemetry stack not properly sized", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_time_buffer.insert(telemetry_stack.top().timestamp);
        unsigned domain_idx;
        size_t offset = 0;
        for (domain_idx = 0; !telemetry_stack.empty(); ++domain_idx) {
            if (telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 0.0 &&
                (m_domain_buffer.size() == 0 ||
                m_domain_buffer.value(0)[offset + GEOPM_TELEMETRY_TYPE_PROGRESS] != 0.0)) {
                m_entry_telemetry[domain_idx] = telemetry_stack.top();
            }
            if (telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_PROGRESS] == 1.0 &&
                (m_domain_buffer.size() == 0 ||
                m_domain_buffer.value(0)[offset + GEOPM_TELEMETRY_TYPE_PROGRESS] != 1.0)) {
                m_is_dirty_domain_sample[domain_idx] = false;
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                    geopm_time_diff(&(m_entry_telemetry[domain_idx].timestamp), &(telemetry_stack.top().timestamp));
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_ENERGY] =
                    (telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] +
                     telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY]) -
                    (m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_PKG_ENERGY] +
                     m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_DRAM_ENERGY]);
                m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_FREQUENCY] =
                    (telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE] -
                     m_entry_telemetry[domain_idx].signal[GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_CORE]) /
                    m_domain_sample[domain_idx].signal[GEOPM_SAMPLE_TYPE_RUNTIME];
            }
            memcpy(m_signal_matrix.data() + offset, telemetry_stack.top().signal, GEOPM_NUM_TELEMETRY_TYPE * sizeof(double));

            std::vector<double> entry_oldest = m_domain_buffer.value(m_domain_buffer.size() - 1);
            if (entry_oldest[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] == 0.0 ||
                m_domain_buffer.size() < m_domain_buffer.capacity()) {
                if (telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != 0.0) {
                    ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
                    ++m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
                }
            }
            else {
                if (telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_RUNTIME] == 0.0) {
                    --m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_PROGRESS];
                    --m_valid_entries[offset + GEOPM_TELEMETRY_TYPE_RUNTIME];
                }
            }

            int num_entries = fmin(m_domain_buffer.size() + 1, m_domain_buffer.capacity());
            std::fill(m_valid_entries.begin() + offset, m_valid_entries.end() + offset + GEOPM_TELEMETRY_TYPE_LLC_VICTIMS, num_entries);

            // Update Min
            for (int i = 0; i < GEOPM_NUM_TELEMETRY_TYPE; ++i) {
                // if i references a valid sample
                if ((i != GEOPM_TELEMETRY_TYPE_RUNTIME &&
                     i != GEOPM_TELEMETRY_TYPE_PROGRESS) ||
                    telemetry_stack.top().signal[GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0) {
                    // Calculate the min
                    if (telemetry_stack.top().signal[i] < m_min[offset + i]) {
                        m_min[offset + i] = telemetry_stack.top().signal[i];
                    }
                    else if (m_domain_buffer.size() < m_domain_buffer.capacity() && m_min[offset + i] == entry_oldest[offset + i]) {
                        //We are about to throw out the current min
                        //Find the new one
                        m_min[offset +i] = telemetry_stack.top().signal[i];
                        for (int entry = 0; entry < m_domain_buffer.size(); ++entry) {
                            bool is_old_value_valid = (i != GEOPM_TELEMETRY_TYPE_RUNTIME &&
                                                       i != GEOPM_TELEMETRY_TYPE_PROGRESS) ||
                                                       m_domain_buffer.value(entry)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                            if (m_domain_buffer.value(entry)[offset + i] < m_min[offset + i] && is_old_value_valid) {
                                m_min[offset +i] = m_domain_buffer.value(entry)[offset + i];
                            }
                        }
                    }
                    // Calculate the max
                    if (telemetry_stack.top().signal[i] > m_max[offset + i]) {
                        m_max[offset + i] = telemetry_stack.top().signal[i];
                    }
                    else if (m_domain_buffer.size() < m_domain_buffer.capacity() && m_max[offset + i] == entry_oldest[offset + i]) {
                        //We are about to throw out the current max
                        //Find the new one
                        m_max[offset +i] = telemetry_stack.top().signal[i];
                        for (int entry = 0; entry < m_domain_buffer.size(); ++entry) {
                            bool is_old_value_valid = (i != GEOPM_TELEMETRY_TYPE_RUNTIME &&
                                                       i != GEOPM_TELEMETRY_TYPE_PROGRESS) ||
                                                       m_domain_buffer.value(entry)[offset + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0;
                            if (m_domain_buffer.value(entry)[offset + i] > m_max[offset + i] && is_old_value_valid) {
                                m_max[offset +i] = m_domain_buffer.value(entry)[offset + i];
                            }
                        }
                    }
                    if (m_domain_buffer.size() < m_domain_buffer.capacity()) {
                        // sum the values
                        m_sum[offset + i] += telemetry_stack.top().signal[i];
                        // sum the square of the values
                        m_sum_squares[offset + i] += pow(telemetry_stack.top().signal[i], 2);
                    }
                    else {
                        // We need to throw away the value of the signal we are removing
                        // sum the values
                        m_sum[offset + i] += (telemetry_stack.top().signal[i] - entry_oldest[offset + i]);
                        // sum the square of the values
                        m_sum_squares[offset + i] += (pow(telemetry_stack.top().signal[i], 2) - pow(entry_oldest[offset + i], 2));
                    }
                }
            }

            offset += GEOPM_NUM_TELEMETRY_TYPE;
            telemetry_stack.pop();
        }

        m_domain_buffer.insert(m_signal_matrix);

        for (domain_idx = 0; !m_is_dirty_domain_sample[domain_idx] && domain_idx != m_num_domain; ++domain_idx) {
            if (domain_idx == m_num_domain) {
                std::fill(m_curr_sample.signal, m_curr_sample.signal + GEOPM_NUM_SAMPLE_TYPE, 0.0);
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
    }


    void Region::insert(const std::vector<struct geopm_sample_message_s> &sample)
    {
        std::fill(m_curr_sample.signal, m_curr_sample.signal + GEOPM_NUM_SAMPLE_TYPE, 0.0);
        size_t offset = 0;
        for (auto it = sample.begin(); it != sample.end(); ++it) {
            memcpy(m_signal_matrix.data() + offset, (*it).signal, GEOPM_NUM_SAMPLE_TYPE * sizeof(double));
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] =
                (*it).signal[GEOPM_SAMPLE_TYPE_RUNTIME] > m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME] ?
                (*it).signal[GEOPM_SAMPLE_TYPE_RUNTIME] : m_curr_sample.signal[GEOPM_SAMPLE_TYPE_RUNTIME];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_ENERGY] += (*it).signal[GEOPM_SAMPLE_TYPE_ENERGY];
            m_curr_sample.signal[GEOPM_SAMPLE_TYPE_FREQUENCY] += (*it).signal[GEOPM_SAMPLE_TYPE_FREQUENCY];

            int num_entries = fmin(m_domain_buffer.size() + 1, m_domain_buffer.capacity());
            std::fill(m_valid_entries.begin(), m_valid_entries.end(), num_entries);

            std::vector<double> entry_oldest = m_domain_buffer.value(m_domain_buffer.size() - 1);
            for (int i = 0; i < GEOPM_NUM_TELEMETRY_TYPE; ++i) {
                // calculate the min
                if ((*it).signal[i] < m_min[offset + i]) {
                    m_min[offset + i] = (*it).signal[i];
                }
                else if (m_domain_buffer.size() == m_domain_buffer.capacity() &&
                         m_min[offset + i] == entry_oldest[offset + i]) {
                    //We are about to throw out the current min
                    //Find the new one
                    m_min[offset +i] = (*it).signal[i];
                    for (int entry = 0; entry < m_domain_buffer.size(); ++entry) {
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
                    for (int entry = 0; entry < m_domain_buffer.size(); ++entry) {
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
            offset += GEOPM_NUM_SAMPLE_TYPE;
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
        double result = NAN;
        if (m_domain_buffer.size()) {
            const std::vector<double> &signal_matrix = m_domain_buffer.value(m_domain_buffer.size() - 1);
            result = signal_matrix[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type];
        }
        return result;
    }

    void Region::statistics(int domain_idx, int signal_type, std::vector<double> &stats) const
    {
        stats[GEOPM_STAT_TYPE_NSAMPLE] = m_valid_entries[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type];
        stats[GEOPM_STAT_TYPE_MEAN] = m_sum[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type] /
                                      stats[GEOPM_STAT_TYPE_NSAMPLE];
        stats[GEOPM_STAT_TYPE_STD_DEV] = m_sum_squares[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type] /
                                         stats[GEOPM_STAT_TYPE_NSAMPLE];
        stats[GEOPM_STAT_TYPE_MIN] = m_min[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type];
        stats[GEOPM_STAT_TYPE_MAX] = m_max[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type];
        std::vector<double> median(stats[GEOPM_STAT_TYPE_NSAMPLE]);
        int idx = 0;
        bool is_hw_telem = signal_type != GEOPM_TELEMETRY_TYPE_PROGRESS || signal_type != GEOPM_TELEMETRY_TYPE_RUNTIME;
        for (int i = 0; i < m_domain_buffer.size(); ++i) {
            if (is_hw_telem ||
                m_domain_buffer.value(i)[GEOPM_NUM_TELEMETRY_TYPE * domain_idx + GEOPM_TELEMETRY_TYPE_RUNTIME] != -1.0) {
                    median[idx++] = m_domain_buffer.value(i)[GEOPM_NUM_TELEMETRY_TYPE * domain_idx + signal_type];
            }
        }
        std::sort(median.begin(), median.begin() + stats[GEOPM_STAT_TYPE_NSAMPLE]);
        stats[GEOPM_STAT_TYPE_MEDIAN] = median[stats[GEOPM_STAT_TYPE_NSAMPLE] / 2];
    }

    double Region::derivative(int domain_idx, int signal_type) const
    {
        double result = NAN;
        if (m_domain_buffer.size() >= 2) {
            const std::vector<double> &signal_matrix_0 = m_domain_buffer.value(m_domain_buffer.size() - 2);
            const std::vector<double> &signal_matrix_1 = m_domain_buffer.value(m_domain_buffer.size() - 1);
            double delta_signal = signal_matrix_1[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type] -
                                  signal_matrix_0[domain_idx * GEOPM_NUM_TELEMETRY_TYPE + signal_type];
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
        return 0.0;
    }
}
