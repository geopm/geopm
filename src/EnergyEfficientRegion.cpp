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
#include "EnergyEfficientRegion.hpp"

#include <cmath>

#include "Agg.hpp"
#include "Helper.hpp"
#include "config.h"

namespace geopm
{
    static size_t calc_num_step(double freq_min, double freq_max, double freq_step)
    {
        return 1 + (size_t)(ceil((freq_max - freq_min) / freq_step));
    }

    EnergyEfficientRegionImp::EnergyEfficientRegionImp(double freq_min, double freq_max, double freq_step)
        : M_PERF_MARGIN(0.10)  // up to 10% degradation allowed
        , M_MAX_INCREASE(4)
        , m_is_learning(true)
        , m_max_step (calc_num_step(freq_min, freq_max, freq_step) - 1)
        , m_freq_step (freq_step)
        , m_curr_step(-1)
        , m_freq_min (freq_min)
        , m_target(0.0)
    {
        /// @brief we are not clearing the m_freq_ctx vector once created, such that we
        ///        do not have to re-learn frequencies that were temporarily removed via
        ///        update_freq_range. so we are assuming that a region's min, max and step
        ///        are whatever is available when it is first observed.  address later.
        for (size_t step = 0; step <= m_max_step; ++step) {
            m_freq_ctx.push_back(geopm::make_unique<FreqContext>());
        }
        update_freq_range(freq_min, freq_max, freq_step);
    }

    void EnergyEfficientRegionImp::update_freq_range(double freq_min, double freq_max, double freq_step)
    {
        if (m_curr_step == -1) {
            /// @todo, should we start at sticker?  sticker - 1?
            m_curr_step = m_max_step;
            m_is_learning = true;
        }
        else {
            throw Exception("EnergyEfficientRegionImp::" + std::string(__func__) + "().",
                            GEOPM_ERROR_NOT_IMPLEMENTED, __FILE__, __LINE__);
        }
    }

    double EnergyEfficientRegionImp::freq(void) const
    {
        return m_freq_min + (m_curr_step * m_freq_step);
    }

    void EnergyEfficientRegionImp::update_exit(double curr_perf_metric)
    {
        if (m_is_learning) {
            auto &curr_freq_ctx = m_freq_ctx[m_curr_step];
            if (!std::isnan(curr_perf_metric) && curr_perf_metric != 0.0) {
                curr_freq_ctx->perf.insert(curr_perf_metric);
            }
            if (curr_freq_ctx->perf.size() >= M_MIN_PERF_SAMPLE) {
                double perf_max = Agg::max(curr_freq_ctx->perf.make_vector());
                if (!std::isnan(perf_max) && perf_max != 0.0) {
                    if (m_target == 0.0) {
                        m_target = (1.0 + M_PERF_MARGIN) * perf_max;
                    }
                    bool do_increase = false;
                    if (m_target != 0.0) {
                        // Performance is in range; lower frequency
                        if (perf_max > m_target) {
                            if (m_curr_step - 1 >= 0) {
                                --m_curr_step;
                            }
                        }
                        else if ((uint64_t) m_curr_step + 1 < m_freq_ctx.size()) {
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
}
