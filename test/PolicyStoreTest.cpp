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

#include "gtest/gtest.h"
#include "PolicyStore.hpp"
#include "Exception.hpp"
#include <limits>
#include <algorithm>

class PolicyStoreTest: public ::testing::Test {};

// Assert that two policies are the same
::testing::AssertionResult PoliciesAreSame(const std::vector<double> &p1, const std::vector<double> &p2)
{
    const std::vector<double> &shorter(p1.size() < p2.size() ? p1 : p2);
    const std::vector<double> &longer(p1.size() >= p2.size() ? p1 : p2);
    size_t index;

    // Within overlapping portions, NaNs result in the same behavior
    for (index = 0; index < shorter.size(); ++index) {
        if (std::isnan(shorter[index]) != std::isnan(longer[index]) &&
            shorter[index] != longer[index]) {
            return ::testing::AssertionFailure() << ::testing::PrintToString(p1)
                   << " does not match " << ::testing::PrintToString(p2);
        }
    }

    // Different-sized policies are the same if they only differ by trailing NaNs
    for (; index < longer.size(); ++index) {
        if (!std::isnan(longer[index])) {
            return ::testing::AssertionFailure() << ::testing::PrintToString(p1)
                   << " does not match " << ::testing::PrintToString(p2);
        }
    }

    return ::testing::AssertionSuccess();
}

TEST_F(PolicyStoreTest, self_consistent)
{
    geopm::PolicyStore policy_store(":memory:");

    // Test that data in = data out, even when some keys are the same
    static const std::vector<double> policy1 = {-2., std::numeric_limits<double>::quiet_NaN(), 6.};
    static const std::vector<double> policy2 = {std::numeric_limits<double>::quiet_NaN(), 1.};
    static const std::vector<double> policy3 = {4.};
    policy_store.set_best("myprofile", "secretagent", policy1);
    policy_store.set_best("myprofile", "anotheragent", policy2);
    policy_store.set_best("anotherprofile", "secretagent", policy3);

    EXPECT_TRUE(PoliciesAreSame(policy1, policy_store.get_best("myprofile", "secretagent")));
    EXPECT_TRUE(PoliciesAreSame(policy2, policy_store.get_best("myprofile", "anotheragent")));
    EXPECT_TRUE(PoliciesAreSame(policy3, policy_store.get_best("anotherprofile", "secretagent")));
}

TEST_F(PolicyStoreTest, update_policy)
{
    geopm::PolicyStore policy_store(":memory:");

    // Test that the latest in a series of set policies is returned on get
    static const std::vector<double> policy1 = {2, 3, 4};
    static const std::vector<double> policy2 = {8, 9, 10};
    policy_store.set_best("myprofile", "secretagent", policy1);
    policy_store.set_best("myprofile", "secretagent", policy2);
    EXPECT_TRUE(PoliciesAreSame(policy2, policy_store.get_best("myprofile", "secretagent")));

    // Test that an entry can be removed
    policy_store.set_best("myprofile", "secretagent", {});
    EXPECT_THROW(policy_store.get_best("myprofile", "secretagent"), geopm::Exception);

    // Test that trailing values no longer exist when an update shrinks the vector
    static const std::vector<double> policy1_trim_end = {2, 3};
    static const std::vector<double> policy1_trim_start = {std::numeric_limits<double>::quiet_NaN(), 3, 4};
    policy_store.set_best("myprofile", "trimend", policy1);
    policy_store.set_best("myprofile", "trimend", policy1_trim_end);
    policy_store.set_best("myprofile", "trimstart", policy1);
    policy_store.set_best("myprofile", "trimstart", policy1_trim_start);
    EXPECT_TRUE(PoliciesAreSame(policy1_trim_end, policy_store.get_best("myprofile", "trimend")));
    EXPECT_TRUE(PoliciesAreSame(policy1_trim_start, policy_store.get_best("myprofile", "trimstart")));
}

TEST_F(PolicyStoreTest, table_precedence)
{
    geopm::PolicyStore policy_store(":memory:");
    static const std::vector<double> default_policy = {2, 3, 4};
    static const std::vector<double> better_policy = {1, 2, 3};
    policy_store.set_default("myagent", default_policy);
    policy_store.set_best("optimizedprofile", "myagent", better_policy);

    // Test that an override is used when present, even if a default is available
    EXPECT_TRUE(PoliciesAreSame(better_policy, policy_store.get_best("optimizedprofile", "myagent")));

    // Test that a default is used in the absence of a best policy
    EXPECT_TRUE(PoliciesAreSame(default_policy, policy_store.get_best("unoptimizedprofile", "myagent")));

    // Test that an exception is thrown when no usable entry exists
    EXPECT_THROW(policy_store.get_best("unoptimizedprofile", "anotheragent"), geopm::Exception);
}

