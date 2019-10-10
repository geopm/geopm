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

#include "geopm_test.hpp"

#include "gtest/gtest.h"

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

IsEqualToPolicyMatcher::IsEqualToPolicyMatcher(const std::vector<double> &expected)
    : m_expected(expected)
{
}

bool IsEqualToPolicyMatcher::MatchAndExplain(std::vector<double> policy,
                                             ::testing::MatchResultListener *listener) const
{
    bool is_match = true;
    if (policy.size() != m_expected.size()) {
        *listener << "expected size " << m_expected.size() << ", got size "
                  << policy.size();
        is_match = false;
    }
    else {
        // Check for elementwise equality, allowing NANs to be equal to each other
        for (size_t i = 0; i < policy.size(); ++i) {
            if (std::isnan(policy[i]) != std::isnan(m_expected[i]) ||
                (!std::isnan(policy[i]) && policy[i] != m_expected[i])) {
                if (!is_match) {
                    *listener << "; ";
                }
                *listener << "expected[" << i << "] = " << m_expected[i]
                          << ", policy[" << i << "] = " << policy[i];
                is_match = false;
            }
        }
    }

    return is_match;
}

// Describes the property of a value matching this matcher.
// Example message: Expected: {...}
void IsEqualToPolicyMatcher::DescribeTo(std::ostream *os) const
{
    *os << ::testing::PrintToString(m_expected);
}

// Describes the property of a value NOT matching this matcher.
// Example message: Expected: not {...}
void IsEqualToPolicyMatcher::DescribeNegationTo(std::ostream *os) const
{
    *os << "not " << ::testing::PrintToString(m_expected);
}

::testing::Matcher<std::vector<double> > IsEqualToPolicy(const std::vector<double> &policy)
{
    return MakeMatcher(new IsEqualToPolicyMatcher(policy));
}
