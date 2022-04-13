/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "EnergyEfficientRegion.hpp"

#include <cmath>

#include "geopm/Agg.hpp"
#include "geopm/Helper.hpp"
#include "config.h"

namespace geopm
{
    std::unique_ptr<EnergyEfficientRegion> EnergyEfficientRegion::make_unique(double freq_min, double freq_max,
                                                                              double freq_step, double perf_margin)
    {
        return geopm::make_unique<EnergyEfficientRegionImp>(freq_min, freq_max,
                                                            freq_step, perf_margin);
    }

    static size_t calc_num_step(double freq_min, double freq_max, double freq_step)
    {
        return 1 + (size_t)(ceil((freq_max - freq_min) / freq_step));
    }

    EnergyEfficientRegionImp::EnergyEfficientRegionImp(double freq_min, double freq_max,
                                                       double freq_step, double perf_margin)
        : M_MIN_PERF_SAMPLE(5)
        , m_is_learning(true)
        , m_max_step(calc_num_step(freq_min, freq_max, freq_step) - 1)
        , m_freq_step(freq_step)
        , m_curr_step(-1)
        , m_freq_min(freq_min)
        , m_target(0.0)
        , m_perf_margin(perf_margin)
    {
        /// @brief we are not clearing the m_freq_perf vector once created, such that we
        ///        do not have to re-learn frequencies that were temporarily removed via
        ///        update_freq_range. so we are assuming that a region's min, max and step
        ///        are whatever is available when it is first observed.  address later.
        for (size_t step = 0; step <= m_max_step; ++step) {
            m_freq_perf.push_back(geopm::make_unique<CircularBuffer<double> >(M_MIN_PERF_SAMPLE));
        }
        update_freq_range(freq_min, freq_max, freq_step);
#ifdef GEOPM_DEBUG
        if (perf_margin < 0.0 || perf_margin > 1.0) {
            throw Exception("EnergyEfficientRegionImp::" + std::string(__func__) + "(): invalid perf_margin",
                             GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
#endif
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
            auto &curr_perf_buffer = m_freq_perf[m_curr_step];
            if (!std::isnan(curr_perf_metric) && curr_perf_metric != 0.0) {
                curr_perf_buffer->insert(curr_perf_metric);
            }
            if (curr_perf_buffer->size() >= M_MIN_PERF_SAMPLE) {
                double perf_max = Agg::max(curr_perf_buffer->make_vector());
                if (!std::isnan(perf_max) && perf_max != 0.0) {
                    if (m_target == 0.0) {
                        m_target = (1.0 + m_perf_margin) * perf_max;
                    }
                    bool do_increase = false;
                    if (m_target != 0.0) {
                        // Performance is in range; lower frequency
                        if (perf_max > m_target) {
                            if (m_curr_step - 1 >= 0) {
                                --m_curr_step;
                            }
                            else {
                                // stop learning at min frequency
                                m_is_learning = false;
                            }
                        }
                        else if ((uint64_t) m_curr_step + 1 <= m_max_step) {
                            do_increase = true;
                        }
                        else {
                            // stop learning at max frequency
                            m_is_learning = false;
                        }
                    }
                    if (do_increase) {
                        m_is_learning = false;
                        m_curr_step++;
                    }
                }
            }
        }
    }

    bool EnergyEfficientRegionImp::is_learning(void) const
    {
        return m_is_learning;
    }
}
