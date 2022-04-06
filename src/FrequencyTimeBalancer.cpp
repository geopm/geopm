/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "FrequencyTimeBalancer.hpp"

#include "geopm/Helper.hpp"
#include "geopm_debug.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace geopm
{
    class FrequencyTimeBalancerImp : public FrequencyTimeBalancer
    {
        public:
            FrequencyTimeBalancerImp() = delete;
            FrequencyTimeBalancerImp(
                double minimum_frequency,
                double maximum_frequency);
            virtual ~FrequencyTimeBalancerImp() = default;

            std::vector<double> balance_frequencies_by_time(
                const std::vector<double> &previous_times,
                const std::vector<double> &previous_control_frequencies,
                const std::vector<double> &previous_achieved_frequencies,
                const std::vector<std::pair<unsigned int, double> > &frequency_limits_by_high_priority_count,
                double low_priority_frequency) override;

            double get_target_time() const override;

            static std::unique_ptr<FrequencyTimeBalancer> make_unique(
                double minimum_frequency,
                double maximum_frequency);
            static std::shared_ptr<FrequencyTimeBalancer> make_shared(
                double minimum_frequency,
                double maximum_frequency);

        private:
            void update_balance_targets(
                const std::vector<double> &previous_times,
                const std::vector<double> &previous_control_frequencies,
                const std::vector<double> &previous_achieved_frequencies,
                const std::vector<std::pair<unsigned int, double> > &frequency_limits_by_high_priority_count,
                double low_priority_frequency);

            std::vector<double> get_balanced_frequencies(
                const std::vector<double> &previous_times,
                const std::vector<double> &previous_control_frequencies,
                const std::vector<double> &previous_achieved_frequencies);

            std::vector<std::vector<int> > m_domain_index_groups;
            double m_minimum_frequency;
            double m_maximum_frequency;
            // Target balancing time
            double m_target_time;
            // High/Low priority cutoff frequenciy
            double m_cutoff_frequency;
            // Index order for argsorting each monitored domain by application
            // lagginess of processes running in that domain.
            std::vector<size_t> m_lagginess_idx;
    };

    FrequencyTimeBalancerImp::FrequencyTimeBalancerImp(
        double minimum_frequency,
        double maximum_frequency)
        : m_minimum_frequency(minimum_frequency)
        , m_maximum_frequency(maximum_frequency)
        , m_target_time(NAN)
        , m_cutoff_frequency(NAN)
    {
    }

    std::vector<double> FrequencyTimeBalancerImp::balance_frequencies_by_time(
        const std::vector<double> &previous_times,
        const std::vector<double> &previous_control_frequencies,
        const std::vector<double> &previous_achieved_frequencies,
        const std::vector<std::pair<unsigned int, double> > &frequency_limits_by_high_priority_count,
        double low_priority_frequency)
    {
        GEOPM_DEBUG_ASSERT(
            (previous_times.size() == previous_control_frequencies.size() &&
             previous_times.size() == previous_achieved_frequencies.size()),
            "FrequencyTimeBalancerImp::balance_frequencies_by_time(): "
            "input vectors must be the same size.");

        // Estimate the target time we should balance against
        update_balance_targets(
            previous_times, previous_control_frequencies,
            previous_achieved_frequencies,
            frequency_limits_by_high_priority_count, low_priority_frequency);

        // Estimate the frequency controls that would achieve our target balancing time
        return get_balanced_frequencies(
            previous_times, previous_control_frequencies,
            previous_achieved_frequencies);
    }

    void FrequencyTimeBalancerImp::update_balance_targets(
        const std::vector<double> &previous_times,
        const std::vector<double> &previous_control_frequencies,
        const std::vector<double> &previous_achieved_frequencies,
        const std::vector<std::pair<unsigned int, double> > &frequency_limits_by_high_priority_count,
        double low_priority_frequency)
    {
        // Perform an argsort, ordered by decreasing lagginess of a domain
        m_lagginess_idx.resize(previous_times.size());
        std::iota(m_lagginess_idx.begin(), m_lagginess_idx.end(), 0);

        auto is_laggier_core_idx =
            [&previous_times, &previous_achieved_frequencies](size_t lhs, size_t rhs) {
                // Place unrecorded times at the end of the sorted collection
                if (std::isnan(previous_times[lhs])) {
                    return false;
                }
                if (std::isnan(previous_times[rhs])) {
                    return true;
                }

                // Sort by cycles in region of interest, rather than by
                // time. Keep the most sensitive one at the front.
                return (previous_times[lhs] * previous_achieved_frequencies[lhs]) >
                       (previous_times[rhs] * previous_achieved_frequencies[rhs]);
            };

        std::sort(m_lagginess_idx.begin(), m_lagginess_idx.end(), is_laggier_core_idx);

        const auto lagger_time = previous_times[m_lagginess_idx[0]];

        // Set the frequency of each index to match the last-observed
        // non-network time of the slowest recently-frequency-unlimited index.
        // We don't want to balance against frequency-limited indices in case
        // a previous frequency limit was set too low, which could set our
        // target performance too low.
        auto target_it = std::find_if(
            m_lagginess_idx.begin(),
            m_lagginess_idx.end(),
            [&previous_control_frequencies, &previous_times, this](size_t frequency_idx) {
                return !std::isnan(previous_times[frequency_idx]) &&
                       previous_control_frequencies[frequency_idx] >= m_maximum_frequency;
            });
        size_t reference_core_idx;
        if (target_it == m_lagginess_idx.end()) {
            reference_core_idx = m_lagginess_idx[0];
        }
        else {
            reference_core_idx = *target_it;
        }

        // From previously-unthrottled cores, match the core with the most
        // time, scaled to the estimated time at its estimated
        // maximum-achievable frequency.
        double balance_target_time = previous_times[reference_core_idx];
        auto reference_core_hp_cutoff = m_minimum_frequency;

        // See if we can opt for an even lower desired time based on
        // prioritized frequency tradeoffs.
        for (const auto &hp_count_frequency : frequency_limits_by_high_priority_count) {
            const size_t hp_count = hp_count_frequency.first;
            const double hp_frequency = hp_count_frequency.second;

            // Estimate achieved vs achievable impacts
            const auto laggiest_high_priority_time =
                lagger_time *
                previous_achieved_frequencies[m_lagginess_idx[0]] /
                hp_frequency;
            if (hp_count < m_lagginess_idx.size()) {
                const auto laggiest_lp_idx = m_lagginess_idx[hp_count];
                const auto laggiest_low_priority_time =
                    previous_times[laggiest_lp_idx] *
                    previous_achieved_frequencies[laggiest_lp_idx] /
                    low_priority_frequency;
                const auto predicted_long_pole = std::max(laggiest_low_priority_time, laggiest_high_priority_time);
                if (predicted_long_pole < balance_target_time) {
                    balance_target_time = predicted_long_pole;
                    reference_core_hp_cutoff = low_priority_frequency;
                }
            }
            else {
                // Does the all-high-priority projection improve our time estimate?
                if (laggiest_high_priority_time < balance_target_time) {
                    balance_target_time = laggiest_high_priority_time;
                    reference_core_hp_cutoff = low_priority_frequency;
                }
            }
        }

        m_cutoff_frequency = reference_core_hp_cutoff;
        m_target_time = balance_target_time;
    }

    std::vector<double> FrequencyTimeBalancerImp::get_balanced_frequencies(
        const std::vector<double> &previous_times,
        const std::vector<double> &previous_control_frequencies,
        const std::vector<double> &previous_achieved_frequencies)
    {
        std::vector<double> desired_frequencies = previous_control_frequencies;

        if (std::find_if(previous_control_frequencies.begin(),
                         previous_control_frequencies.end(),
                         [this](double frequency) {
                             return frequency >= m_maximum_frequency;
                         }) == previous_control_frequencies.end()) {
            // The previous iteration had no unlimited cores. Return to
            // baseline so we can make a better-informed decision next
            // iteration.
            std::fill(desired_frequencies.begin(),
                      desired_frequencies.end(),
                      m_maximum_frequency);
            return desired_frequencies;
        }

        // Select the frequency that results in the target balanced time
        // for each frequency control domain index.
        double max_group_frequency = m_minimum_frequency;
        for (size_t ctl_idx = 0; ctl_idx < m_lagginess_idx.size(); ++ctl_idx) {
            double desired_frequency =
                previous_achieved_frequencies[ctl_idx] *
                previous_times[ctl_idx] /
                m_target_time;
            bool is_lp = desired_frequency <= m_cutoff_frequency;
            if (is_lp) {
                desired_frequency = std::min(desired_frequency, m_cutoff_frequency);
            }

            if (!std::isnan(previous_times[ctl_idx]) && !std::isnan(desired_frequency)) {
                desired_frequencies[ctl_idx] =
                    std::min(m_maximum_frequency, std::max(m_minimum_frequency, desired_frequency));
                max_group_frequency = std::max(max_group_frequency, desired_frequencies[ctl_idx]);
            }
        }

        if (max_group_frequency < m_maximum_frequency) {
            auto frequency_scale = m_maximum_frequency / max_group_frequency;
            // Scale up only the cores that we want to be high priority.
            // Scale them far enough that the highest-frequency one is at
            // the maximum allowed frequency
            for (size_t ctl_idx = 0; ctl_idx < m_lagginess_idx.size(); ++ctl_idx) {
                const auto ordered_ctl_idx = m_lagginess_idx[ctl_idx];
                if (!std::isnan(previous_times[ordered_ctl_idx]) && !std::isnan(desired_frequencies[ordered_ctl_idx])) {
                    if (desired_frequencies[ordered_ctl_idx] > m_cutoff_frequency) {
                        desired_frequencies[ordered_ctl_idx] =
                            std::min(m_maximum_frequency,
                                     std::max(m_minimum_frequency,
                                              desired_frequencies[ordered_ctl_idx] * frequency_scale));
                    }
                    else {
                        desired_frequencies[ordered_ctl_idx] =
                            std::min(m_cutoff_frequency,
                                     std::max(m_minimum_frequency,
                                              desired_frequencies[ordered_ctl_idx] * frequency_scale));
                    }
                }
            }
        }

        return desired_frequencies;
    }

    double FrequencyTimeBalancerImp::get_target_time() const
    {
        return m_target_time;
    }

    std::unique_ptr<FrequencyTimeBalancer> FrequencyTimeBalancer::make_unique(
        double minimum_frequency,
        double maximum_frequency)
    {
        return geopm::make_unique<FrequencyTimeBalancerImp>(
            minimum_frequency,
            maximum_frequency);
    }

    std::shared_ptr<FrequencyTimeBalancer> FrequencyTimeBalancer::make_shared(
        double minimum_frequency,
        double maximum_frequency)
    {
        return std::make_shared<FrequencyTimeBalancerImp>(
            minimum_frequency,
            maximum_frequency);
    }
}
