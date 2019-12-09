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

#include "Policy.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

/*
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
*/

TEST(PolicyTest, construct_from_vector)
{

}

TEST(PolicyTest, construct_from_agent_name)
{
    // instead of constructor, use factory function
}

TEST(PolicyTest, fill_nans)
{
    // requires agent name
}

TEST(PolicyTest, set_field)
{
    // requires agent name
}

TEST(PolicyTest, equality)
{
    // {4, 5} == {4, 5}
    // {5, 4, 3} != {NAN, 4, 3}
    // {2, 7} != {2}
    // {6, 5} == {6, 5, NAN}


}

TEST(PolicyTest, to_string)
{

}

TEST(PolicyTest, to_json_string)
{
    // requires agent name to be passed
}
