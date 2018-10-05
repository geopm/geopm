/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
#include <cmath>

#include "Agg.hpp"
#include "EnergyEfficientRegion.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"
#include <iostream>
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    static size_t calc_num_step(double freq_min, double freq_max, double freq_step)
    {
        return 1 + (size_t)(ceil((freq_max - freq_min) / freq_step));
    }

    EnergyEfficientRegion::EnergyEfficientRegion(IPlatformIO &platform_io,
                                                 double freq_min, double freq_max, double freq_step,
                                                 int runtime_idx,
                                                 int pkg_energy_idx)
        : M_PERF_MARGIN(0.10)  // up to 10% degradation allowed
        , M_ENERGY_MARGIN(0.025)
        , M_MIN_BASE_SAMPLE(4)
        , M_MAX_INCREASE(4)
        , m_platform_io(platform_io)
        , m_is_learning(true)
        , m_max_step (calc_num_step(freq_min, freq_max, freq_step) - 1)
        , m_freq_step (freq_step)
        , m_freq_min (freq_min)
        , m_freq_max (freq_min + (m_max_step * m_freq_step))
        , m_curr_step(-1)
        , m_target(0.0)
        , m_curr_perf(NAN)
        , m_start_energy(0.0)
        , m_curr_energy(NAN)
        , m_runtime_idx(runtime_idx)
        , m_pkg_energy_idx(pkg_energy_idx)
    {
        /// @brief we are not clearing the m_freq_ctx vector once created, such that we
        ///        do not have to re-learn frequencies that were temporarily removed via
        ///        update_freq_range. so we are assuming that a region's min, max and step
        ///        are whatever is available when it is first observed.  address later.
        for (size_t step = 0; step <= m_max_step; ++step) {
            m_freq_ctx.push_back(geopm::make_unique<FreqContext>(M_MIN_BASE_SAMPLE));
        }
        update_freq_range(freq_min, freq_max, freq_step);
    }

    void EnergyEfficientRegion::update_freq_range(double freq_min, double freq_max, double freq_step)
    {
        if (m_curr_step == -1) {
            m_curr_step = m_max_step;
            m_is_learning = true;
        }
        else {
            double curr_freq = freq_min + (m_curr_step * m_freq_step);
            if (curr_freq < freq_min) {
                /// todo calculate step
                m_curr_step = 0;
            } else if (curr_freq > m_freq_max) {
                m_curr_step = calc_num_step(freq_min, freq_max, freq_step) - 1;
            }
            if (m_freq_ctx[m_curr_step]->num_increase == M_MAX_INCREASE) {
                m_is_learning = false;
            }
        }
    }

    double EnergyEfficientRegion::perf_metric()
    {
        // Higher is better for performance, so negate
        return -1.0 * m_platform_io.sample(m_runtime_idx);
    }

    double EnergyEfficientRegion::energy_metric()
    {
        return m_platform_io.sample(m_pkg_energy_idx);
    }

    double EnergyEfficientRegion::freq(void) const
    {
        return m_freq_min + (m_curr_step * m_freq_step);
    }

    void EnergyEfficientRegion::update_entry()
    {
        m_start_energy = energy_metric();
    }

    void EnergyEfficientRegion::update_exit()
    {
        m_curr_perf = perf_metric();
        m_curr_energy = energy_metric() - m_start_energy;
        if (m_is_learning) {
            auto &curr_freq_ctx = m_freq_ctx[m_curr_step];
            if (!std::isnan(m_curr_perf) && m_curr_perf != 0.0 &&
                !std::isnan(m_curr_energy) && m_curr_energy != 0.0) {
                curr_freq_ctx->perf_buff.insert(m_curr_perf);
                curr_freq_ctx->energy_buff.insert(m_curr_energy);
                curr_freq_ctx->count++;
            }
            if (curr_freq_ctx->count % M_MIN_BASE_SAMPLE == 0) {
                curr_freq_ctx->count = 0;
                size_t step_up  = m_curr_step + 1;
                double curr_med_perf = Agg::median(curr_freq_ctx->perf_buff.make_vector());
                double curr_med_energy = Agg::median(curr_freq_ctx->energy_buff.make_vector());
                if (!std::isnan(curr_med_perf) &&
                    m_target == 0.0) {
                    m_target = (1.0 + M_PERF_MARGIN) * curr_med_perf;
                }

                bool do_increase = false;
                if (step_up < m_freq_ctx.size()) {
                    // assume best min energy is at highest freq if energy follows cpu-bound
                    // pattern; otherwise, energy should decrease with frequency.
                    const auto &step_up_freq_ctx = m_freq_ctx[step_up];
                    double step_up_med_energy = Agg::median(step_up_freq_ctx->energy_buff.make_vector());
                    if (step_up_med_energy < (1.0 - M_ENERGY_MARGIN) * curr_med_energy) {
                        do_increase = true;
                    }
                }
                if (m_target != 0.0) {
                    // Performance is in range; lower frequency
                    if (curr_med_perf > m_target) {
                        if (m_curr_step - 1 >= 0) {
                            m_curr_step = m_curr_step - 1;
                        }
                    }
                    else {
                        do_increase = true;
                    }
                }

                if (do_increase) {
                    // Performance degraded too far; increase freq
                    ++curr_freq_ctx->num_increase;
                    // If the frequency has been lowered too far too
                    // many times, stop learning
                    if (curr_freq_ctx->num_increase == M_MAX_INCREASE) {
                        m_is_learning = false;
                    }
                    m_curr_step++;
                }
            }
        }
    }
}
