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
#include "config.h"

#include "FrequencyTimeBalancer.hpp"

#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

#include <algorithm>
#include <numeric>
#include <cmath>

namespace geopm
{
    class FrequencyTimeBalancerImp : public FrequencyTimeBalancer
    {
        public:
            FrequencyTimeBalancerImp() = delete;
            FrequencyTimeBalancerImp(
                double uncertainty_window_seconds,
                int subdomain_group_count,
                std::function<bool(int)> do_ignore_domain_idx,
                double minimum_frequency,
                double maximum_frequency);
            virtual ~FrequencyTimeBalancerImp() = default;

            std::vector<double> balance_frequencies_by_time(
                    const std::vector<double>& previous_times,
                    const std::vector<double>& previous_frequencies) override;

            static std::unique_ptr<FrequencyTimeBalancer> make_unique(
                    double uncertainty_window_seconds,
                    int subdomain_group_count,
                    std::function<bool(int)> do_ignore_domain_idx,
                    double minimum_frequency,
                    double maximum_frequency);
            static std::shared_ptr<FrequencyTimeBalancer> make_shared(
                    double uncertainty_window_seconds,
                    int subdomain_group_count,
                    std::function<bool(int)> do_ignore_domain_idx,
                    double minimum_frequency,
                    double maximum_frequency);
        private:
            double m_uncertainty_window_seconds;
            int m_subdomain_group_count;
            std::function<bool(int)> m_do_ignore_domain_idx;
            double m_minimum_frequency;
            double m_maximum_frequency;
    };


    FrequencyTimeBalancerImp::FrequencyTimeBalancerImp(
            double uncertainty_window_seconds,
            int subdomain_group_count,
            std::function<bool(int)> do_ignore_domain_idx,
            double minimum_frequency,
            double maximum_frequency)
        : m_uncertainty_window_seconds(uncertainty_window_seconds)
        , m_subdomain_group_count(subdomain_group_count)
        , m_do_ignore_domain_idx(do_ignore_domain_idx)
        , m_minimum_frequency(minimum_frequency)
        , m_maximum_frequency(maximum_frequency)
    {
    }

    std::vector<double> FrequencyTimeBalancerImp::balance_frequencies_by_time(
                const std::vector<double>& previous_times,
                const std::vector<double>& previous_frequencies)
    {
        GEOPM_DEBUG_ASSERT(previous_times.size() == previous_frequencies.size(),
                           "FrequencyTimeBalancerImp::balance_frequencies_by_time(): "
                           "time vector must be the same size as the frequency vector.");

        const size_t domain_count_per_group =
            previous_frequencies.size() / m_subdomain_group_count;
        std::vector<double> desired_frequencies = previous_frequencies;

        std::vector<size_t> idx(previous_times.size());
        std::iota(idx.begin(), idx.end(), 0);

        // Perform an argsort, ordered by decreasing lagginess of a domain
        for (size_t group_idx = 0; group_idx < static_cast<size_t>(m_subdomain_group_count); ++group_idx) {
            std::sort(
                idx.begin() + group_idx * domain_count_per_group,
                idx.begin() + (group_idx + 1) * domain_count_per_group,
                [this, &previous_times](size_t lhs, size_t rhs) {
                    // Place unrecorded times at the end of the sorted collection
                    if (std::isnan(previous_times[lhs]) || m_do_ignore_domain_idx(lhs)) {
                        return false;
                    }
                    if (std::isnan(previous_times[rhs]) || m_do_ignore_domain_idx(rhs)) {
                        return true;
                    }

                    return previous_times[lhs] > previous_times[rhs];
                });
        }

        // Select a balancing reference for each group
        std::vector<size_t> group_leader_idx;
        for (size_t group_idx = 0;
             group_idx < static_cast<size_t>(m_subdomain_group_count); ++group_idx) {
            size_t local_leader_idx = (group_idx + 1) * domain_count_per_group - 1;
            // The domain index that makes the fastest progress is the last one
            // in the sorted collection, for the indices that have a time.
            while ((m_do_ignore_domain_idx(idx[local_leader_idx])
                        || std::isnan(previous_times[idx[local_leader_idx]])
                        || std::isinf(previous_times[idx[local_leader_idx]]))
                    && local_leader_idx > group_idx * domain_count_per_group) {
                local_leader_idx -= 1;
            }
            group_leader_idx.push_back(local_leader_idx);
        }

        for (size_t group_idx = 0;
             group_idx < static_cast<size_t>(m_subdomain_group_count); ++group_idx) {
            const auto group_idx_offset = group_idx * domain_count_per_group;
            const auto next_group_idx_offset = (group_idx + 1) * domain_count_per_group;

            if (std::find_if(previous_frequencies.begin() + group_idx_offset,
                             previous_frequencies.begin() + next_group_idx_offset,
                             [this](double frequency) {
                                 return frequency >= m_maximum_frequency;
                             }) == previous_frequencies.begin() + next_group_idx_offset) {
                // The previous iteration had no unlimited cores. Return to
                // baseline so we can make a better-informed decision next
                // iteration.
                std::fill(desired_frequencies.begin() + group_idx_offset,
                          desired_frequencies.begin() + next_group_idx_offset,
                          m_maximum_frequency);
                continue;
            }

            const auto leader_idx = group_leader_idx[group_idx];//*std::min_element(group_leader_idx.begin(), group_leader_idx.end());
            const auto leader_time = previous_times[idx[leader_idx]];
            const auto lagger_time = previous_times[idx[group_idx_offset]];
            if (leader_time > lagger_time - m_uncertainty_window_seconds) {
                // This group was already balanced well enough in the previous
                // iteration, so keep the same frequency settings.
                continue;
            }

            // Set the frequency of each index to match the last-observed
            // non-network time of the slowest recently-frequency-unlimited index.
            // We don't want to balance against frequency-limited indices in case
            // a previous frequency limit was set too low, which could set our
            // target performance too low.
            size_t target_idx = group_idx_offset;
            for (; target_idx < next_group_idx_offset; ++target_idx) {
                if (previous_frequencies[idx[target_idx]] >= m_maximum_frequency) {
                    // We are iterating over a sorted index where at least one
                    // record must always have max frequency, so the first hit
                    // in this search is the worst-performing match.
                    break;
                }
            }
            const double desired_time = previous_times[idx[target_idx]];

            // Select the frequency that results in the target balanced time
            // for each frequency control domain index.
            for (size_t group_ctl_idx = group_idx_offset; group_ctl_idx < next_group_idx_offset; ++group_ctl_idx) {
                const size_t ctl_idx = idx[group_ctl_idx];
                double desired_frequency =
                    (previous_times[ctl_idx] <= m_uncertainty_window_seconds) ? previous_frequencies[ctl_idx] : // Last epoch was all time from balancer-excluded regions. Don't try to project.
                    (desired_time - previous_times[ctl_idx] <= 0) ? m_maximum_frequency : // Right on the critical path? Aggressively speed up.
                    (desired_time - previous_times[ctl_idx] <= m_uncertainty_window_seconds) ? previous_frequencies[ctl_idx] : // Near the critical path? Stay here.
                    previous_frequencies[ctl_idx]
                    * (previous_times[ctl_idx])
                    / (desired_time);
                if (!m_do_ignore_domain_idx(ctl_idx) && !std::isnan(desired_frequency)) {
                    desired_frequencies[ctl_idx] =
                        std::min(m_maximum_frequency, std::max(m_minimum_frequency, desired_frequency));
                }
            }

            auto hp_core_it = std::find_if(
                    desired_frequencies.begin() + group_idx_offset,
                    desired_frequencies.begin() + next_group_idx_offset,
                    [this](double frequency) {
                        return frequency >= m_maximum_frequency;
                    });
            if (hp_core_it == desired_frequencies.begin() + next_group_idx_offset) {
                // Other parts of this function assume there is always at least
                // one unlimited core. This (along with the initially unlimited
                // state) guarantees it by resetting to baseline configuration.
                // The next decision after this one will be able to adjust.
                std::fill(desired_frequencies.begin() + group_idx_offset,
                          desired_frequencies.begin() + next_group_idx_offset,
                          m_maximum_frequency);
            }
        }

        return desired_frequencies;
    }

    std::unique_ptr<FrequencyTimeBalancer> FrequencyTimeBalancer::make_unique(
                double uncertainty_window_seconds,
                int subdomain_group_count,
                std::function<bool(int)> do_ignore_domain_idx,
                double minimum_frequency,
                double maximum_frequency)
    {
        return geopm::make_unique<FrequencyTimeBalancerImp>(
                uncertainty_window_seconds,
                subdomain_group_count,
                do_ignore_domain_idx,
                minimum_frequency,
                maximum_frequency);
    }

    std::shared_ptr<FrequencyTimeBalancer> FrequencyTimeBalancer::make_shared(
                double uncertainty_window_seconds,
                int subdomain_group_count,
                std::function<bool(int)> do_ignore_domain_idx,
                double minimum_frequency,
                double maximum_frequency)
    {
        return std::make_shared<FrequencyTimeBalancerImp>(
                uncertainty_window_seconds,
                subdomain_group_count,
                do_ignore_domain_idx,
                minimum_frequency,
                maximum_frequency);
    }
}
