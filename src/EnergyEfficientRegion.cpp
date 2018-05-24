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

#include "EnergyEfficientRegion.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include "Exception.hpp"
#include "config.h"

namespace geopm
{

    EnergyEfficientRegion::EnergyEfficientRegion(IPlatformIO &platform_io,
                                                 double freq_min, double freq_max,
                                                 double freq_step,
                                                 int runtime_idx,
                                                 int pkg_energy_idx,
                                                 int dram_energy_idx)
        : m_platform_io(platform_io)
        , M_NUM_FREQ(1 + (size_t)(ceil((freq_max-freq_min)/freq_step)))
        , m_curr_idx(M_NUM_FREQ - 1)
        , m_num_increase(M_NUM_FREQ, 0)
        , m_allowed_freq(M_NUM_FREQ)
        , m_perf_max(M_NUM_FREQ, 0)
        , m_energy_min(M_NUM_FREQ, 0)
        , m_num_sample(M_NUM_FREQ, 0)
        , m_runtime_idx(runtime_idx)
        , m_pkg_energy_idx(pkg_energy_idx)
        , m_dram_energy_idx(dram_energy_idx)
    {
        // set up allowed frequency range
        double freq = freq_min;
        for (auto &freq_it : m_allowed_freq) {
            freq_it = freq;
            freq += freq_step;
        }
    }

    double EnergyEfficientRegion::perf_metric()
    {
        double runtime = m_platform_io.sample(m_runtime_idx);
        if (std::isnan(runtime)) {
            runtime = 0.0;
        }
        // Higher is better for performance, so negate
        return -1.0 * runtime;
    }

    double EnergyEfficientRegion::energy_metric()
    {
        double total_energy = 0.0;
        total_energy += m_platform_io.sample(m_pkg_energy_idx);
        total_energy += m_platform_io.sample(m_dram_energy_idx);
        return total_energy;
    }

    double EnergyEfficientRegion::freq(void) const
    {
        return m_allowed_freq[m_curr_idx];
    }

    void EnergyEfficientRegion::update_entry()
    {
        m_start_energy = energy_metric();
    }

    void EnergyEfficientRegion::update_exit()
    {
        if (m_is_learning) {
            double perf = perf_metric();
            double energy = energy_metric() - m_start_energy;
            if (!std::isnan(perf) && !std::isnan(energy)) {
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
                // assume best min energy is at highest freq if energy follows cpu-bound
                // pattern; otherwise, energy should decrease with frequency.
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
