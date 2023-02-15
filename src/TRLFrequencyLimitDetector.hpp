/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TRLFREQUENCYLIMITDETECTOR_HPP_INCLUDE
#define TRLFREQUENCYLIMITDETECTOR_HPP_INCLUDE

#include <FrequencyLimitDetector.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace geopm
{
    // A frequency limit detector that depends on CPU package turbo ratio limits
    class TRLFrequencyLimitDetector : public FrequencyLimitDetector
    {
        public:
            TRLFrequencyLimitDetector(
                PlatformIO &platform_io,
                const PlatformTopo &);

            void update_max_frequency_estimates(
                const std::vector<double> &observed_core_frequencies) override;
            std::vector<std::pair<unsigned int, double> > get_core_frequency_limits(
                unsigned int core_idx) const override;
            double get_core_low_priority_frequency(unsigned int core_idx) const override;

        private:
            unsigned int m_package_count;
            size_t m_core_count;
            const double M_CPU_FREQUENCY_MAX;
            const double M_CPU_FREQUENCY_STICKER;
            std::vector<std::vector<int> > m_cores_in_packages;
            std::vector<std::vector<std::pair<unsigned int, double> > > m_core_frequency_limits;
            std::vector<double> m_core_lp_frequencies;
    };
}

#endif
