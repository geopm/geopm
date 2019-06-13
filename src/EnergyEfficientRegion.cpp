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
        , m_is_learning(true)
        , m_max_step(calc_num_step(freq_min, freq_max, freq_step) - 1)
        , m_freq_step(freq_step)
        , m_curr_step(-1)
        , m_freq_min(freq_min)
        , m_target(0.0)
        , m_is_disabled(false)
        , m_perf_max(NAN)
    {
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

    void EnergyEfficientRegionImp::sample(double curr_perf_metric)
    {
        if (!std::isnan(curr_perf_metric) && curr_perf_metric != 0.0) {
            m_perf.push_back(curr_perf_metric);
        }
    }

    void EnergyEfficientRegionImp::update_exit()
    {
        if (m_is_learning && !m_is_disabled) {
            m_perf_max = Agg::max(m_perf);
            m_perf.clear();
            if (!std::isnan(m_perf_max) && m_perf_max != 0.0) {
                if (m_target == 0.0) {
                    // @todo when dynamic freq ranges are enabled setting target
                    // needs to be moved to a function and reused here and in update_freq_range
                    m_target = (1.0 + M_PERF_MARGIN) * m_perf_max;
                }
                else {
                    // Performance is in range; lower frequency
                    if (m_perf_max > m_target) {
                        if (m_curr_step - 1 >= 0) {
                            --m_curr_step;
                        }
                        else {
                            m_is_learning = false;
                        }
                    }
                    else if ((uint64_t) m_curr_step + 1 <= m_max_step) {
                        m_curr_step++;
                        m_is_learning = false;
                    }
                }

            }
        }
    }

    void EnergyEfficientRegionImp::disable(void)
    {
        m_is_disabled = true;
    }

    bool EnergyEfficientRegionImp::is_learning(void) const
    {
        return m_is_learning;
    }
}
