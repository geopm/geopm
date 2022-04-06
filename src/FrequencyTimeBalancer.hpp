/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FREQUENCYTIMEBALANCER_HPP_INCLUDE
#define FREQUENCYTIMEBALANCER_HPP_INCLUDE

#include <memory>
#include <vector>

namespace geopm
{
    /// Select frequency control settings that are expected to balance measured
    /// execution times. Assumes time impact of up to (frequency_old/frequency_new)
    /// percent. Workloads less frequency-sensitive than that should be able to
    /// go lower than the recommended frequencies. This is expected to converge
    /// toward those lower frequencies if it is repeatedly re-evaluated some
    /// time after applying the recommended frequency controls.
    class FrequencyTimeBalancer
    {
        public:
            FrequencyTimeBalancer() = default;
            virtual ~FrequencyTimeBalancer() = default;

            /// @brief Return the recommended frequency controls given observed
            ///        times while operating under a given set of previous
            ///        frequency controls. The returned vector is the same size
            ///        as the input vectors.
            /// @param previous_times  Time spent in the region to be balanced,
            ///        measured by any domain.
            /// @param previous_control_frequencies  Frequency control last applied
            ///        over the region to be balanced, measured by the same
            ///        domain as \p previous_times.
            /// @param previous_achieved_frequencies  Average observed frequencies
            ///        over the region to be balanced, measured by the same
            ///        domain as \p previous_times.
            virtual std::vector<double> balance_frequencies_by_time(
                const std::vector<double> &previous_times,
                const std::vector<double> &previous_control_frequencies,
                const std::vector<double> &previous_achieved_frequencies,
                const std::vector<std::pair<unsigned int, double> > &frequency_limits_by_high_priority_count,
                double low_priority_frequency) = 0;

            /// @brief return the target time last used to balance the frequencies.
            virtual double get_target_time() const = 0;

            /// @brief Allocate a FrequencyTimeBalancer instance.
            /// @param minimum_frequency  The lowest frequency control to allow
            ///        in rebalancing frequency control decisions.
            /// @param maximum_frequency  The highest frequency control to allow
            ///        in rebalancing frequency control decisions.
            static std::unique_ptr<FrequencyTimeBalancer> make_unique(
                double minimum_frequency,
                double maximum_frequency);

            /// \copydoc FrequencyTimeBalancer::make_unique
            static std::shared_ptr<FrequencyTimeBalancer> make_shared(
                double minimum_frequency,
                double maximum_frequency);
    };
}

#endif /* FREQUENCYTIMEBALANCER_HPP_INCLUDE */
