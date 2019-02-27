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
///@todo only need for debug at the moment...
#include "Exception.hpp"
#include "config.h"

namespace geopm
{
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
        , m_curr_step(-1)
        , m_target(0.0)
        , m_start_energy(0.0)
        , m_runtime_idx(runtime_idx)
        , m_pkg_energy_idx(pkg_energy_idx)
    {
        update_freq_range(freq_min, freq_max, freq_step);
    }

    void EnergyEfficientRegion::update_freq_range(double freq_min, double freq_max, double freq_step)
    {
        /// @todo m_freq_step == freq_step else we have to re-key our map
        ///       or make m_freq_step const

        const struct m_freq_ctx_s freq_ctx_stub = {.num_increase = 0,
                                                   .energy = 0,
                                                   .perf_buff = CircularBuffer<double>(),
                                                   .energy_buff = CircularBuffer<double>(),};
        // set up allowed frequency range
        m_freq_min = freq_min;
        m_freq_step = freq_step;
        size_t num_freq_step = 1 + (size_t)(ceil((freq_max - freq_min) / m_freq_step));
        m_freq_ctx_map.clear();
        size_t step;
        for (step = 0; step < num_freq_step; ++step) {
            auto it = m_freq_ctx_map.emplace(std::piecewise_construct,
                                   std::make_tuple(step),
                                   std::make_tuple(freq_ctx_stub));
            it.first->second.perf_buff.set_capacity(M_MIN_BASE_SAMPLE);
            it.first->second.energy_buff.set_capacity(M_MIN_BASE_SAMPLE);
        }
        m_max_step = step;
        m_freq_max = freq_min + (m_max_step * m_freq_step);
        double curr_freq = freq_min + (m_curr_step * m_freq_step);
        if (m_curr_step == -1) {
            m_curr_step = 0;
            m_is_learning = true;
            curr_freq = m_freq_max;
        } else if (curr_freq < freq_min) {
            m_curr_step = 0;
            if (m_freq_ctx_map.find(m_curr_step)->second.num_increase == M_MAX_INCREASE) {
                m_is_learning = false;
            }
        } else if (curr_freq > m_freq_max) {
            m_curr_step = m_max_step;
            if (m_freq_ctx_map.find(m_curr_step)->second.num_increase == M_MAX_INCREASE) {
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
        auto curr_freq_ctx = m_freq_ctx_map.find(m_curr_step);
        auto step_up_freq_ctx_it = m_freq_ctx_map.find(m_curr_step + 1);
        if (m_is_learning) {
            double curr_perf = perf_metric();
            double curr_energy = energy_metric() - m_start_energy;
            double curr_med_perf = Agg::median(curr_freq_ctx->second.perf_buff.make_vector());
            //double curr_med_energy = Agg::median(curr_freq_ctx->second.energy_buff.make_vector());
                if (!std::isnan(curr_med_perf)) {

                    if (curr_med_perf > 0.0) {
                        m_target = (1.0 - M_PERF_MARGIN) * curr_med_perf;
                    }
                    else {
                        m_target = (1.0 + M_PERF_MARGIN) * curr_med_perf;
                    }
                }

            bool do_increase = false;
            if (step_up_freq_ctx_it != m_freq_ctx_map.end()) {
                // assume best min energy is at highest freq if energy follows cpu-bound
                // pattern; otherwise, energy should decrease with frequency.
                const auto &step_up_freq_ctx = step_up_freq_ctx_it->second;
                if (step_up_freq_ctx.energy <
                    (1.0 - M_ENERGY_MARGIN) * curr_freq_ctx->second.energy) {
                    do_increase = true;
                }
            }
            if (m_target != 0.0) {
                if (curr_med_perf > m_target) {
                    uint64_t next_step = m_curr_step - 1;
                    if (m_freq_ctx_map.find(next_step) != m_freq_ctx_map.end()) {
                        // Performance is in range; lower frequency
                        m_curr_step = next_step;
                    }
                }
                else {
                    do_increase = true;
                }
            }

            if (do_increase) {
                // Performance degraded too far; increase freq
                ++curr_freq_ctx->second.num_increase;
                // If the frequency has been lowered too far too
                // many times, stop learning
                if (curr_freq_ctx->second.num_increase == M_MAX_INCREASE) {
                    m_is_learning = false;
                }
                m_curr_step++;
            }
            if (!std::isnan(curr_perf) && curr_perf != 0.0) {
                curr_freq_ctx->second.perf_buff.insert(curr_perf);
            }
            if (!std::isnan(curr_energy) && curr_energy != 0.0) {
                curr_freq_ctx->second.energy_buff.insert(curr_energy);
                curr_freq_ctx->second.energy = curr_energy;
            }
        }
    }

    std::vector<std::string> EnergyEfficientRegion::trace_names(void) const
    {
        return {"m_is_learning", "m_curr_step", "m_target", "num_increase",
                "perf_metric", "energy_metric", "med_filt_perf_metric", "med_filt_energy_metric"};
    }

    void EnergyEfficientRegion::trace_values(std::vector<double> &values)
    {
        auto curr_freq_ctx = m_freq_ctx_map.find(m_curr_step);
        double perf = perf_metric();
        double energy = energy_metric() - m_start_energy;
        double med_perf = Agg::median(curr_freq_ctx->second.perf_buff.make_vector());
        double med_energy = Agg::median(curr_freq_ctx->second.energy_buff.make_vector());
        values[TRACE_COL_M_IS_LEARNING] = m_is_learning;
        values[TRACE_COL_M_CURR_STEP] = m_curr_step;
        values[TRACE_COL_M_TARGET] = m_target;
        values[TRACE_COL_NUM_INCREASE] = curr_freq_ctx->second.num_increase;
        values[TRACE_COL_PERF_METRIC] = perf;
        values[TRACE_COL_ENERGY_METRIC] = energy;
        values[TRACE_COL_MED_FILT_PERF_METRIC] = med_perf;
        values[TRACE_COL_MED_FILT_ENERGY_METRIC] = med_energy;
    }
}
