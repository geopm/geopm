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

#include "AdaptiveFreqRegion.hpp"

#include <cassert>

namespace geopm
{

    AdaptiveFreqRegion::AdaptiveFreqRegion(geopm::IRegion *region,
                                           double freq_min, double freq_max,
                                           double freq_step)
        : m_region(region),
          m_num_freq(1 + (size_t)(ceil((freq_max-freq_min)/freq_step))),
          m_curr_idx(m_num_freq - 1),
          m_allowed_freq(m_num_freq),
          m_perf_total(m_num_freq, 0),
          m_num_sample(m_num_freq, 0)
    {
        assert(nullptr != m_region);

        // set up allowed frequency range
        double freq = freq_min;
        for (auto &freq_it : m_allowed_freq) {
            freq_it = freq;
            freq += freq_step;
        }
    }

    AdaptiveFreqRegion::~AdaptiveFreqRegion()
    {

    }

    double AdaptiveFreqRegion::perf_metric()
    {
        double current_time = m_region->signal(0, GEOPM_TELEMETRY_TYPE_CLK_UNHALTED_REF);
        double elapsed = NAN;
        if (m_start_time > 0.0) {
            // higher perf is better, so negate runtime
            elapsed = -1.0 * (current_time - m_start_time);
        }
        m_start_time = current_time;
        return elapsed;
    }

    double AdaptiveFreqRegion::freq(void)
    {
        return m_allowed_freq[m_curr_idx];
    }

    void AdaptiveFreqRegion::update(void)
    {
        if (m_is_learning) {
            double perf = perf_metric();
            if (!isnan(perf)) {
                m_perf_total[m_curr_idx] += perf;
                m_num_sample[m_curr_idx] += 1;
            }

            if (m_num_sample[m_curr_idx] > 0) {
                double average_perf = m_perf_total[m_curr_idx] / m_num_sample[m_curr_idx];
                if (m_num_sample[m_curr_idx] >= m_min_base_sample &&
                    m_target == 0.0 &&
                    m_curr_idx == m_num_freq-1) {

                    if (average_perf > 0.0) {
                        m_target = (1.0 - m_target_ratio) * average_perf;
                    }
                    else {
                        m_target = (1.0 + m_target_ratio) * average_perf;
                    }
                }

                if (m_target != 0.0) {
                    if (average_perf > m_target) {
                        if (m_curr_idx > 0) {
                            // Performance is in range; lower frequency
                            --m_curr_idx;
                        }
                    } else {
                        if (m_curr_idx < m_num_freq - 1) {
                            // Performance degraded too far; increase freq
                            ++m_curr_idx;
                        }
                        ++m_num_increase;
                        // If the frequency has been lowered too far too
                        // many times, stop learning
                        if (m_num_increase == m_max_increase) {
                            m_is_learning = false;
                        }
                    }
                }
            }
        }

    }

}
