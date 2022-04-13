/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENERGYEFFICIENTREGION_HPP_INCLUDE
#define ENERGYEFFICIENTREGION_HPP_INCLUDE

#include <cmath>
#include <set>
#include <vector>
#include <memory>

#include "geopm/CircularBuffer.hpp"

namespace geopm
{
    /// @brief Holds the performance history of a Region.
    class EnergyEfficientRegion
    {
        public:
            virtual ~EnergyEfficientRegion() = default;
            virtual double freq(void) const = 0;
            virtual void update_freq_range(double freq_min, double freq_max, double freq_step) = 0;
            virtual void update_exit(double curr_perf_metric) = 0;
            virtual bool is_learning(void) const = 0;
            static std::unique_ptr<EnergyEfficientRegion> make_unique(double freq_min, double freq_max,
                                                                      double freq_step, double perf_margin);
    };

    class EnergyEfficientRegionImp : public EnergyEfficientRegion
    {
        public:
            EnergyEfficientRegionImp(double freq_min, double freq_max,
                                     double freq_step, double perf_margin);
            virtual ~EnergyEfficientRegionImp() = default;
            double freq(void) const override;
            void update_freq_range(double freq_min, double freq_max, double freq_step) override;
            void update_exit(double curr_perf_metric) override;
            bool is_learning(void) const override;
        private:
            const int M_MIN_PERF_SAMPLE;
            bool m_is_learning;
            uint64_t m_max_step;
            double m_freq_step;
            int m_curr_step;
            double m_freq_min;
            double m_target;
            std::vector<std::unique_ptr<CircularBuffer<double> > > m_freq_perf;
            double m_perf_margin;
    };

} // namespace geopm

#endif
