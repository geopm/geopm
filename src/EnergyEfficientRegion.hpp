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

#ifndef ENERGYEFFICIENTREGION_HPP_INCLUDE
#define ENERGYEFFICIENTREGION_HPP_INCLUDE

#include <set>
#include <vector>
#include <memory>

#include "CircularBuffer.hpp"

namespace geopm
{
    class PlatformIO;

    /// @brief Holds the performance history of a Region.
    class EnergyEfficientRegion
    {
        public:
            EnergyEfficientRegion(PlatformIO &platform_io,
                                  double freq_min, double freq_max, double freq_step,
                                  int runtime_idx);
            virtual ~EnergyEfficientRegion() = default;
            double freq(void) const;
            void update_freq_range(double freq_min, double freq_max, double freq_step);
            void update_entry(void);
            void update_exit(void);
        private:
            struct FreqContext {
                FreqContext(uint64_t buffer_size)
                    : count(0)
                    , num_increase(0)
                {
                    perf_buff.set_capacity(buffer_size);
                };

                virtual ~FreqContext() = default;
                int count;
                size_t num_increase;
                CircularBufferImp<double> perf_buff;
            };

            // Used to determine whether performance degraded or not.
            // Higher is better.
            double perf_metric();

            const double M_PERF_MARGIN;
            const size_t M_MIN_BASE_SAMPLE;
            const size_t M_MAX_INCREASE;

            PlatformIO &m_platform_io;
            bool m_is_learning;
            uint64_t m_max_step;
            double m_freq_step;
            int m_curr_step;
            double m_freq_min;
            double m_freq_max;
            double m_target;
            double m_curr_perf;

            int m_runtime_idx;
            std::vector<std::unique_ptr<FreqContext> > m_freq_ctx;
    };

} // namespace geopm

#endif
