/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

namespace geopm
{

    AdaptiveFreqRegion::AdaptiveFreqRegion(geopm::IRegion *region,
                                           double freq_min, double freq_max,
                                           double freq_step, int num_domain)
        : m_region(region)
        , M_NUM_FREQ(1 + (size_t)(ceil((freq_max-freq_min)/freq_step)))
        , m_curr_idx(M_NUM_FREQ - 1)
        , m_num_increase(M_NUM_FREQ, 0)
        , m_allowed_freq(M_NUM_FREQ)
        , m_perf_max(M_NUM_FREQ, 0)
        , m_energy_min(M_NUM_FREQ, 0)
        , m_num_sample(M_NUM_FREQ, 0)
        , m_start_time({0, 0})
        , m_num_domain(num_domain)
    {
        if (nullptr == region) {
            throw Exception("AdaptiveFreqRegion(): region cannot be NULL",
                            GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        // set up allowed frequency range
        double freq = freq_min;
        for (auto &freq_it : m_allowed_freq) {
            freq_it = freq;
            freq += freq_step;
        }
    }

    double AdaptiveFreqRegion::perf_metric()
    {
        double elapsed = NAN;
        struct geopm_time_s current_time = m_region->telemetry_timestamp(-1);
        struct geopm_time_s zero{};
        if (geopm_time_comp(&zero, &m_start_time)) {
            // higher perf is better, so negate runtime
            elapsed = -1.0 * geopm_time_diff(&m_start_time, &current_time);
        }
        return elapsed;
    }

    double AdaptiveFreqRegion::energy_metric()
    {
        double total_energy = 0.0;
        for (int domain_idx = 0; domain_idx != m_num_domain; ++domain_idx) {
            total_energy += m_region->signal(domain_idx, GEOPM_TELEMETRY_TYPE_PKG_ENERGY);
            total_energy += m_region->signal(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
        }
        return total_energy;
    }

    double AdaptiveFreqRegion::freq(void) const
    {
        return m_allowed_freq[m_curr_idx];
    }

    void AdaptiveFreqRegion::update_entry()
    {
        struct geopm_time_s current_time = m_region->telemetry_timestamp(-1);
        m_start_time = current_time;
        m_start_energy = energy_metric();
    }

    void AdaptiveFreqRegion::update_exit()
    {
        if (m_is_learning) {
            double perf = perf_metric();
            double energy = energy_metric() - m_start_energy;
            if (!isnan(perf) && !isnan(energy)) {
                if (m_num_sample[m_curr_idx] == 0 ||
                    m_perf_max[m_curr_idx] < perf) {
                    m_perf_max[m_curr_idx] = perf;
                }
                if (m_num_sample[m_curr_idx] == 0 ||
                    m_energy_min[m_curr_idx] > energy) {
                    m_energy_min[m_curr_idx] = energy;
                }
                m_num_sample[m_curr_idx] += 1;
            }

            if (m_num_sample[m_curr_idx] > 0) {
                if (m_num_sample[m_curr_idx] >= M_MIN_BASE_SAMPLE &&
                    m_target == 0.0 &&
                    m_curr_idx == M_NUM_FREQ - 1) {

                    if (m_perf_max[m_curr_idx] > 0.0) {
                        m_target = (1.0 - M_PERF_MARGIN) * m_perf_max[m_curr_idx];
                    }
                    else {
                        m_target = (1.0 + M_PERF_MARGIN) * m_perf_max[m_curr_idx];
                    }
                }

                bool do_increase = false;
                if (m_curr_idx != M_NUM_FREQ - 1 &&
                    m_energy_min[m_curr_idx + 1] < (1.0 - M_ENERGY_MARGIN) * m_energy_min[m_curr_idx]) {
                    do_increase = true;
                }
                else if (m_target != 0.0) {
                    if (m_perf_max[m_curr_idx] > m_target) {
                        if (m_curr_idx > 0) {
                            // Performance is in range; lower frequency
                            --m_curr_idx;
                        }
                    }
                    else {
                        if (m_curr_idx != M_NUM_FREQ - 1) {
                            do_increase = true;
                        }
                    }
                }
                if (do_increase) {
                    // Performance degraded too far; increase freq
                    ++m_num_increase[m_curr_idx];
                    // If the frequency has been lowered too far too
                    // many times, stop learning
                    if (m_num_increase[m_curr_idx] == M_MAX_INCREASE) {
                        m_is_learning = false;
                    }
                    ++m_curr_idx;
                }
            }
        }
    }

}
