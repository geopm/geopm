/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FREQUENCYLIMITDETECTOR_HPP_INCLUDE
#define FREQUENCYLIMITDETECTOR_HPP_INCLUDE

#include <memory>
#include <utility>
#include <vector>

namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    /// @brief Detect maximum achievable frequency limits of CPU cores.
    /// @details Estimates the maximum freuqency that each core can achieve if
    /// it is given a sufficiently high frequency cap. Estimates are based on
    /// recent behavior of the target core and other cores in the same CPU
    /// package.
    class FrequencyLimitDetector
    {
        public:
            FrequencyLimitDetector() = default;
            virtual ~FrequencyLimitDetector() = default;

            /// @brief Update the estimates for maximum achievable core frequencies.
            /// @details Caches the estimates to be queried by other functions
            /// in this interface.
            /// @param observed_core_frequencies The measured frequency for
            /// each core across a region of interest (e.g., epoch to epoch,
            /// across GEOPM regions, etc).
            virtual void update_max_frequency_estimates(
                const std::vector<double> &observed_core_frequencies) = 0;

            /// @brief Estimate the maximum achievable frequencies of a given core.
            /// @param [in] core_idx GEOPM topology index of the core to query.
            /// @return A vector of alternative frequency configurations. Each
            /// vector element is a pair of a count of high-priority cores in
            /// the package, and this core's achievable frequency if that count
            /// is not exceeded.
            virtual std::vector<std::pair<unsigned int, double> > get_core_frequency_limits(
                unsigned int core_idx) const = 0;

            /// @brief Estimate the low priority frequency of a given core.
            virtual double get_core_low_priority_frequency(
                unsigned int core_idx) const = 0;

            static std::unique_ptr<FrequencyLimitDetector> make_unique(
                PlatformIO &platform_io, const PlatformTopo &platform_topo);

            static std::shared_ptr<FrequencyLimitDetector> make_shared(
                PlatformIO &platform_io, const PlatformTopo &platform_topo);
    };
}

#endif
