/*
 * Copyright (c) 2015 - 2022, Intel Corporation
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
#ifndef FREQUENCYTIMEBALANCER_HPP_INCLUDE
#define FREQUENCYTIMEBALANCER_HPP_INCLUDE

#include <memory>
#include <functional>
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
            virtual ~FrequencyTimeBalancer() = default;

            /// @brief Return the recommended frequency controls given observed
            ///        times while operating under a given set of previous
            ///        frequency controls.
            /// @param previous_times  Time spent in the region to be balanced,
            ///        measured by any domain.
            /// @param previous_frequencies  Frequency control last applied
            ///        over the region to be balanced, measured by the same
            ///        domain as \p previous_times.
            virtual std::vector<double> balance_frequencies_by_time(
                    const std::vector<double>& previous_times,
                    const std::vector<double>& previous_frequencies) = 0;

            /// @brief Allocate a FrequencyTimeBalancer instance.
            /// @param uncertainty_window_seconds  Width of a window of time
            ///        within which the balancer can consider two measured times
            ///        to be in balance with each other.
            /// @param sumdomain_group_count  Number of subdomains within the
            ///        domains to be balanced. E.g., to independently balance
            ///        each package within per-core times and frequencies,
            ///        this should be set to the number of packages on the node.
            /// @param do_ignore_domain_idx  A function that accepts an argument
            ///        indicating the domain index of a time to be balanced. The
            ///        function should return true if that domain index should
            ///        be excluded from balancing decisions. In that case, the
            ///        previous frequency for that index will be recommended as
            ///        the next frequency for that index.
            /// @param minimum_frequency  The lowest frequency control to allow
            ///        in rebalancing frequency control decisions.
            /// @param maximum_frequency  The highest frequency control to allow
            ///        in rebalancing frequency control decisions.
            static std::unique_ptr<FrequencyTimeBalancer> make_unique(
                    double uncertainty_window_seconds,
                    int subdomain_group_count,
                    std::function<bool(int)> do_ignore_domain_idx,
                    double minimum_frequency,
                    double maximum_frequency);
            /// \copydoc FrequencyTimeBalancer::make_unique
            static std::shared_ptr<FrequencyTimeBalancer> make_shared(
                    double uncertainty_window_seconds,
                    int subdomain_group_count,
                    std::function<bool(int)> do_ignore_domain_idx,
                    double minimum_frequency,
                    double maximum_frequency);
    };
}

#endif
