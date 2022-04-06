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

#include <memory>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "FrequencyTimeBalancer.hpp"

using geopm::FrequencyTimeBalancer;
using ::testing::DoubleEq;
using ::testing::Pointwise;

class FrequencyTimeBalancerTest : public ::testing::Test
{
};

TEST_F(FrequencyTimeBalancerTest, balance_when_current_frequencies_are_all_unlimited)
{
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);
    const std::vector<double> previous_frequencies = { 3e9, 3e9, 3e9, 3e9 };
    // For all subtests, the outputs must contain at least one value at the max
    // frequency.

    // Single long-running core
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 1.0, 1.0, 2.0, 1.0 },
                                                      previous_frequencies),
                Pointwise(DoubleEq(), { 1.5e9, 1.5e9, 3e9, 1.5e9 }));

    // Stratified run times
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 1.0, 2.0, 3.0, 4.0 },
                                                      previous_frequencies),
                Pointwise(DoubleEq(), { 1e9 /* 0.75e9, clamped to freq min */, 1.5e9, 2.25e9, 3e9 }));
}

TEST_F(FrequencyTimeBalancerTest, balance_when_all_frequencies_should_go_unlimited)
{
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);
    const std::vector<double> desired_frequencies = { 3e9, 3e9, 3e9, 3e9 };
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 1.33,    1.6, 2.0, 4.0 },
                                                      { 3.0e9, 2.5e9, 2e9, 1e9 }),
                Pointwise(DoubleEq(), desired_frequencies));
}

TEST_F(FrequencyTimeBalancerTest, does_not_change_when_already_balanced)
{
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);

    const std::vector<double> balanced_frequencies = { 1e9, 3e9, 2e9, 1e9 };
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 2.0, 2.0, 2.0, 2.0 },
                                                      balanced_frequencies),
                Pointwise(DoubleEq(), balanced_frequencies));
}

TEST_F(FrequencyTimeBalancerTest, detects_nearly_balanced)
{
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);

    // Just barely balanced
    {
        const double low_time = 0.1;
        const double high_time = low_time + 0.004;
        EXPECT_THAT(balancer->balance_frequencies_by_time({ low_time, low_time, low_time, high_time },
                                                          { 2e9, 2e9, 2e9, 3e9 }),
                    Pointwise(DoubleEq(), { 2e9, 2e9, 2e9, 3e9 }))
            << "Already balanced: frequencies should be unchanged";
    }

    // Just barely not balanced
    {
        const double low_time = 0.1;
        const double high_time = low_time + 0.006;
        const double time_ratio = low_time / high_time;
        EXPECT_THAT(balancer->balance_frequencies_by_time({ low_time, low_time, low_time, high_time },
                                                          { 3e9, 3e9, 3e9, 3e9 }),
                    Pointwise(DoubleEq(), { 3e9 * time_ratio, 3e9 * time_ratio, 3e9 * time_ratio, 3e9 }))
            << "Barely not balanced: Leader cores should be slowed down";
    }
}

TEST_F(FrequencyTimeBalancerTest, independently_balances_subdomains)
{
    // E.g., Separately balancing 2 packages across all cores
    const int subgroup_count = 2;
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, subgroup_count, [](int) { return false; }, 1e9, 3e9);

    {
        const double low_time = 0.1;
        const double high_time = 2 * low_time;
        const double time_ratio = low_time / high_time;
        EXPECT_THAT(balancer->balance_frequencies_by_time({ high_time, low_time, // Group 1
                                                            low_time, high_time }, // Group 2
                                                          { 3e9, 3e9,
                                                            3e9, 3e9 }),
                    Pointwise(DoubleEq(), { 3e9,              3e9 * time_ratio,
                                            3e9 * time_ratio, 3e9               }))
            << "Multiple subgroups should be balanced separately";
    }
}

TEST_F(FrequencyTimeBalancerTest, can_ignore_control_domain_indices)
{
    // E.g., may use to ignore a core that should not be balanced (such as a
    // a core in a non-application region)
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int control_index) { return control_index == 0; }, 1e9, 3e9);

    // Ignore the lagger core
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 4.0, 3.0, 2.0, 1.0 },
                                                      { 2e9 /* ignored */, 3e9, 3e9, 3e9 }),
                Pointwise(DoubleEq(), { 2e9, 3e9, 3e9 * 2 / 3, 3e9 * 1 / 3 }))
        << "Expected to ignore index 0 (lagger core)";

    // Ignore the leader core core
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 1.0, 2.0, 3.0, 4.0 },
                                                      { 2e9 /* ignored */, 2e9, 3e9, 3e9 }),
                Pointwise(DoubleEq(), { 2e9, 1e9 /* half */, 3e9 * 3 / 4, 3e9 }))
        << "Expected to ignore index 0 (leader core)";
}

TEST_F(FrequencyTimeBalancerTest, does_not_use_capped_cores_as_balance_reference)
{
    // E.g., What if the previous control decision was a misjudgement?
    // If the lagger core is also a freqeuncy-limited core, we shouldn't use it
    // as a balancing reference. Otherwise, we could get stuck in a descent.
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);

    // Just one core had a bad setting:
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 4.0, 3.0, 2.0, 1.0 },
                                                      { 1e9, 3e9, 3e9, 3e9 }),
                // Lagger is at 4 seconds, but is also frequency-capped. So
                // treat the next-lagging uncapped time (3.0) as our reference.
                Pointwise(DoubleEq(), { 3e9, 3e9, 3e9 * 2/3, 3e9 / 3}))
        << "Expected to balance against the most-lagging frequency-unlimited core "
           "(time=3.0) and aggressively unlimit other laggers";

    // More than one core had a bad setting:
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 4.0, 3.0, 2.0, 1.0 },
                                                      { 1e9, 1e9, 3e9, 3e9 }),
                // Top 2 laggers are frequency-capped. So treat the
                // next-lagging uncapped time (2.0) as our reference.
                Pointwise(DoubleEq(), { 3e9, 3e9, 3e9, 3e9 / 2}))
        << "Expected to balance against the most-lagging frequency-unlimited core "
           "(time=2.0) and aggressively unlimit other laggers";
}

TEST_F(FrequencyTimeBalancerTest, resets_to_baseline_if_invariants_are_violated)
{
    // This helper always chooses at least one unlimited frequency, and its
    // performance objective assumes that there is at least always one
    // unlimited frequency. If for any reason that is not true, the helper
    // should fix that issue in the current decision.
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 5e9);

    // No cores currently have a max frequency control (5e9) setting
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 4.0, 3.0, 2.0, 1.0 },
                                                      { 4e9, 3e9, 2e9, 1e9 }),
                Pointwise(DoubleEq(), { 5e9, 5e9, 5e9, 5e9 }))
        << "Expected to reset all cores to unlimited frequency when the previous "
           "state had no frequency-unlimited cores.";
}

TEST_F(FrequencyTimeBalancerTest, no_time_spent_in_balancing_regions)
{
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);

    EXPECT_THAT(balancer->balance_frequencies_by_time({ 0, 0, 0, 0 },
                                                      { 3e9, 2e9, 2e9, 2e9 }),
                Pointwise(DoubleEq(), { 3e9, 2e9, 2e9, 2e9 }))
        << "Expected unchanged frequencies when no cores have time data.";

    EXPECT_THAT(balancer->balance_frequencies_by_time({ 3, 2, 1, 0 },
                                                      { 3e9, 2e9, 3e9, 2e9 }),
                Pointwise(DoubleEq(), { 3e9, 2e9 * 2/3, 3e9 / 3, 2e9 }))
        << "Expected a core with no time data to be have unchanged frequency.";
}

TEST_F(FrequencyTimeBalancerTest, negative_time_spent_in_balancing_regions)
{
    // Negative time? This can occur when deriving a time signal by subtracting
    // one noisy time signal from another noisy time signal. Make sure we can
    // handle it since it seems likely our callers may encounter this.
    auto balancer = FrequencyTimeBalancer::make_unique(
        0.005, 1, [](int) { return false; }, 1e9, 3e9);

    // All negative times
    EXPECT_THAT(balancer->balance_frequencies_by_time({ -1, -2, -3, -4 },
                                                      { 3e9, 2e9, 2e9, 2e9 }),
                Pointwise(DoubleEq(), { 3e9, 2e9, 2e9, 2e9 }))
        << "Expected to balance against the greatest time when all times are negative.";

    // Some negative times
    EXPECT_THAT(balancer->balance_frequencies_by_time({ 3, 2, 1, -1 },
                                                      { 3e9, 2e9, 3e9, 2e9 }),
                Pointwise(DoubleEq(), { 3e9, 2e9 * 2/3, 3e9 / 3, 2e9 }))
        << "Expected a core with negative time data to be have unchanged frequency.";
}
