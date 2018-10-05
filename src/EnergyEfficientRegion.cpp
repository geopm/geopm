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
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
    EnergyEfficientRegion::EnergyEfficientRegion(IPlatformIO &platform_io,
                                                 int runtime_idx,
                                                 int pkg_energy_idx)
        : M_PERF_MARGIN(0.10)  // up to 10% degradation allowed
        , M_ENERGY_MARGIN(0.025)
        , M_MIN_BASE_SAMPLE(4)
        , M_MAX_INCREASE(4)
        , m_platform_io(platform_io)
        , m_curr_freq(NAN)
        , m_target(0.0)
        , m_is_learning(true)
        , m_start_energy(0.0)
        , m_runtime_idx(runtime_idx)
        , m_pkg_energy_idx(pkg_energy_idx)
    {

    }

    void EnergyEfficientRegion::update_freq_range(const double freq_min, const double freq_max, const double freq_step)
    {
        /// @todo m_freq_step == freq_step else we have to re-key our map
        ///       or make m_freq_step const

        const struct m_freq_ctx_s freq_ctx_stub = {.num_increase = 0};
        // set up allowed frequency range
        m_freq_step = freq_step;
        double num_freq_step = 1 + (size_t)(ceil((freq_max - freq_min) / m_freq_step));
        m_allowed_freq.clear();
        double freq = 0.0;
        for (double step = 0; step < num_freq_step; ++step) {
            freq = freq_min + (step * m_freq_step);
            m_allowed_freq.insert(freq);
            auto it = m_freq_ctx_map.emplace(std::piecewise_construct,
                                   std::make_tuple(freq / m_freq_step),
                                   std::make_tuple(freq_ctx_stub));
            it.first->second.perf.set_capacity(M_MIN_BASE_SAMPLE);
            it.first->second.energy.set_capacity(M_MIN_BASE_SAMPLE);
        }
        m_curr_freq_max = freq;
        if (std::isnan(m_curr_freq)) {
            m_is_learning = true;
            m_curr_freq = m_curr_freq_max;
        } else if (m_curr_freq < *m_allowed_freq.begin()) {
            m_curr_freq = *m_allowed_freq.begin();
            if (m_freq_ctx_map[m_curr_freq / m_freq_step].num_increase == M_MAX_INCREASE) {
                m_is_learning = false;
            }
        } else if (m_curr_freq > m_curr_freq_max) {
            m_curr_freq = m_curr_freq_max;
            if (m_freq_ctx_map[m_curr_freq / m_freq_step].num_increase == M_MAX_INCREASE) {
                m_is_learning = false;
            }
        }
    }

    double EnergyEfficientRegion::perf_metric()
    {
        double runtime = m_platform_io.sample(m_runtime_idx);
        // Higher is better for performance, so negate
        return -1.0 * runtime;
    }

    double EnergyEfficientRegion::energy_metric()
    {
        return m_platform_io.sample(m_pkg_energy_idx);
    }

    double EnergyEfficientRegion::freq(void) const
    {
        return m_curr_freq;
    }

    void EnergyEfficientRegion::update_entry()
    {
        m_start_energy = energy_metric();
    }

    void EnergyEfficientRegion::update_exit()
    {
        auto &curr_freq_ctx = m_freq_ctx_map[m_curr_freq / m_freq_step];
        auto step_up_freq_ctx_it = m_freq_ctx_map.find((m_curr_freq + m_freq_step) / m_freq_step);
        if (m_is_learning) {
            double perf = perf_metric();
            double energy = energy_metric() - m_start_energy;
            if (!std::isnan(perf) && !std::isnan(energy) &&
                perf != 0.0 && energy != 0.0) {
                curr_freq_ctx.perf.insert(perf);
                curr_freq_ctx.energy.insert(energy);
            }
            double med_perf = Agg::median(curr_freq_ctx.perf.make_vector());
            double med_energy = Agg::median(curr_freq_ctx.energy.make_vector());
            if (curr_freq_ctx.perf.size() > 0) {
                if ((size_t) curr_freq_ctx.perf.size() >= M_MIN_BASE_SAMPLE &&
                    m_target == 0.0 &&
                    m_curr_freq == m_curr_freq_max) {

                    if (med_perf > 0.0) {
                        m_target = (1.0 - M_PERF_MARGIN) * med_perf;
                    }
                    else {
                        m_target = (1.0 + M_PERF_MARGIN) * med_perf;
                    }
                }

                bool do_increase = false;
                // assume best min energy is at highest freq if energy follows cpu-bound
                // pattern; otherwise, energy should decrease with frequency.
                const auto &step_up_freq_ctx = step_up_freq_ctx_it->second;
                if (m_curr_freq != m_curr_freq_max &&
                    Agg::median(step_up_freq_ctx.energy.make_vector()) <
                    (1.0 - M_ENERGY_MARGIN) * med_energy) {
                    do_increase = true;
                }
                else if (m_target != 0.0) {
                    if (med_perf > m_target) {
                        double next_freq = m_curr_freq - m_freq_step;
                        if (m_allowed_freq.find(next_freq) != m_allowed_freq.end()) {
                            // Performance is in range; lower frequency
                            m_curr_freq = next_freq;
                        }
                    }
                    else {
                        double next_freq = m_curr_freq + m_freq_step;
                        if (m_allowed_freq.find(next_freq) != m_allowed_freq.end()) {
                            do_increase = true;
                        }
                    }
                }

                if (do_increase) {
                    // Performance degraded too far; increase freq
                    ++curr_freq_ctx.num_increase;
                    // If the frequency has been lowered too far too
                    // many times, stop learning
                    if (curr_freq_ctx.num_increase == M_MAX_INCREASE) {
                        m_is_learning = false;
                    }
                    m_curr_freq += m_freq_step;
                }
            }
        }
    }
}
