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

#include "PolicyStoreImp.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Agent.hpp"
#include "Exception.hpp"
#include "MockAgent.hpp"
#include "Helper.hpp"

class PolicyStoreImpTest : public ::testing::Test
{
    protected:
       void SetUp();
       std::vector<std::string> m_agent_policy_names = { "first", "second", "third" };
};

void PolicyStoreImpTest::SetUp()
{
    try {
        geopm::agent_factory().register_plugin(
            "agent_without_policy",
            []() { return geopm::make_unique<MockAgent>(); },
            geopm::Agent::make_dictionary({}, {}));

        geopm::agent_factory().register_plugin(
            "agent_with_policy", []() { return geopm::make_unique<MockAgent>(); },
            geopm::Agent::make_dictionary(m_agent_policy_names, {}));

        geopm::agent_factory().register_plugin(
            "another_agent_with_policy",
            []() { return geopm::make_unique<MockAgent>(); },
            geopm::Agent::make_dictionary(m_agent_policy_names, {}));
    }
    catch (const geopm::Exception &e) {
        // There is no inverse to register_plugin(), so we can't clean
        // up between tests. Ignore exceptions that can result from
        // re-registering these between tests
    }
}

// Assert that two policies are the same
::testing::AssertionResult
PoliciesAreSame(const std::vector<double> &p1, const std::vector<double> &p2)
{
    const std::vector<double> &shorter(p1.size() < p2.size() ? p1 : p2);
    const std::vector<double> &longer(p1.size() >= p2.size() ? p1 : p2);
    size_t index;

    // Within overlapping portions, NaNs result in the same behavior
    for (index = 0; index < shorter.size(); ++index) {
        if (std::isnan(shorter[index]) != std::isnan(longer[index]) &&
            shorter[index] != longer[index]) {
            return ::testing::AssertionFailure()
                   << ::testing::PrintToString(p1) << " does not match "
                   << ::testing::PrintToString(p2);
        }
    }

    // Different-sized policies are the same if they only differ by trailing NaNs
    for (; index < longer.size(); ++index) {
        if (!std::isnan(longer[index])) {
            return ::testing::AssertionFailure()
                   << ::testing::PrintToString(p1) << " does not match "
                   << ::testing::PrintToString(p2);
        }
    }

    return ::testing::AssertionSuccess();
}

TEST_F(PolicyStoreImpTest, self_consistent)
{
    geopm::PolicyStoreImp policy_store(":memory:");

    // Test that data in = data out, even when some keys are the same
    static const std::vector<double> policy1 = { -2., NAN, 6. };
    static const std::vector<double> policy2 = { NAN, 1. };
    static const std::vector<double> policy3 = { 4. };
    policy_store.set_best("myprofile", "agent_with_policy", policy1);
    policy_store.set_best("myprofile", "another_agent_with_policy", policy2);
    policy_store.set_best("anotherprofile", "agent_with_policy", policy3);

    EXPECT_TRUE(PoliciesAreSame(
        policy1, policy_store.get_best("myprofile", "agent_with_policy")));
    EXPECT_TRUE(PoliciesAreSame(
        policy2, policy_store.get_best("myprofile", "another_agent_with_policy")));
    EXPECT_TRUE(PoliciesAreSame(
        policy3, policy_store.get_best("anotherprofile", "agent_with_policy")));
}

TEST_F(PolicyStoreImpTest, update_policy)
{
    geopm::PolicyStoreImp policy_store(":memory:");

    // Test that the latest in a series of set policies is returned on get
    static const std::vector<double> policy1 = { 2, 3, 4 };
    static const std::vector<double> policy2 = { 8, 9, 10 };
    policy_store.set_best("myprofile", "agent_with_policy", policy1);
    policy_store.set_best("myprofile", "agent_with_policy", policy2);
    EXPECT_TRUE(PoliciesAreSame(
        policy2, policy_store.get_best("myprofile", "agent_with_policy")));

    // Test that an entry can be removed
    policy_store.set_best("myprofile", "agent_with_policy", {});
    EXPECT_THROW(policy_store.get_best("myprofile", "agent_with_policy"), geopm::Exception);

    // Test that trailing values no longer exist when an update shrinks the vector
    static const std::vector<double> policy1_trim_end = { 2, 3 };
    static const std::vector<double> policy1_trim_start = { NAN, 3, 4 };
    policy_store.set_best("trimend", "agent_with_policy", policy1);
    policy_store.set_best("trimend", "agent_with_policy", policy1_trim_end);
    policy_store.set_best("trimstart", "agent_with_policy", policy1);
    policy_store.set_best("trimstart", "agent_with_policy", policy1_trim_start);
    EXPECT_TRUE(PoliciesAreSame(policy1_trim_end,
                                policy_store.get_best("trimend", "agent_with_policy")));
    EXPECT_TRUE(PoliciesAreSame(policy1_trim_start,
                                policy_store.get_best("trimstart", "agent_with_policy")));
}

TEST_F(PolicyStoreImpTest, table_precedence)
{
    geopm::PolicyStoreImp policy_store(":memory:");
    static const std::vector<double> agent_default_policy = { NAN };
    static const std::vector<double> configured_default_policy = { 2, 3, 4 };
    static const std::vector<double> better_policy = { 1, 2, 3 };

    policy_store.set_default("agent_with_policy", configured_default_policy);
    policy_store.set_best("optimizedprofile", "agent_with_policy", better_policy);

    // Test that an override is used when present, even if a default is available
    EXPECT_TRUE(PoliciesAreSame(
        better_policy, policy_store.get_best("optimizedprofile", "agent_with_policy")));

    // Test that a default is used in the absence of a best policy
    EXPECT_TRUE(PoliciesAreSame(
        configured_default_policy,
        policy_store.get_best("unoptimizedprofile", "agent_with_policy")));

    // Test that it is possible to specify an override that bypasses the
    // PolicyStore default in favor of the agent's defaults.
    policy_store.set_best("optimizedprofile", "agent_with_policy", agent_default_policy);
    auto best_policy = policy_store.get_best("optimizedprofile", "agent_with_policy");
    EXPECT_TRUE(PoliciesAreSame(agent_default_policy, best_policy));
    EXPECT_EQ(best_policy.size(), m_agent_policy_names.size());

    // Test that an empty policy is returned when no policies are specified, but
    // the agent doesn't use a policy anyways.
    EXPECT_TRUE(PoliciesAreSame(
        {}, policy_store.get_best("unoptimizedprofile", "agent_without_policy")));

    // Test that an exception is thrown when no usable entry exists, and the
    // agent expects a policy
    policy_store.set_best("unoptimizedprofile", "agent_with_policy", {});
    policy_store.set_default("agent_with_policy", {});
    EXPECT_THROW(policy_store.get_best("unoptimizedprofile", "agent_with_policy"),
                 geopm::Exception);
}

