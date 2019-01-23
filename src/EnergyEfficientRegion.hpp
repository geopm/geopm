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

#include <map>
#include <set>
#include <vector>

namespace geopm
{

    class IPlatformIO;

    /// @brief Holds the performance history of a Region.
    class EnergyEfficientRegion
    {
        public:
            EnergyEfficientRegion(IPlatformIO &platform_io,
                                  int runtime_idx,
                                  int pkg_energy_idx);
            virtual ~EnergyEfficientRegion() = default;
            double freq(void) const;
            void update_freq_range(const double freq_min, const double freq_max, const double freq_step);
            void update_entry(void);
            void update_exit(void);
        private:
            // Used to determine whether performance degraded or not.
            // Higher is better.
            virtual double perf_metric();
            virtual double energy_metric();

            IPlatformIO &m_platform_io;
            size_t m_curr_idx;
            double m_curr_freq = NAN;
            double m_target = 0.0;
            const double M_PERF_MARGIN = 0.10;  // up to 10% degradation allowed
            const double M_ENERGY_MARGIN = 0.025;
            const size_t M_MIN_BASE_SAMPLE = 4;

            bool m_is_learning;
            struct m_freq_ctx_s {
                size_t num_increase;
                double perf_max;
                double energy_min;
                size_t num_sample;
            };

            std::map<size_t, struct m_freq_ctx_s> m_freq_ctx_map;
            const size_t M_MAX_INCREASE = 4;

            double m_freq_step;
            std::set<double> m_allowed_freq;
            double m_curr_freq_max;
            double m_start_energy = 0.0;

            int m_runtime_idx;
            int m_pkg_energy_idx;
    };

} // namespace geopm

#endif
