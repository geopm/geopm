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

#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "Policy.hpp"
#include "Exception.hpp"
#include "geopm_test.hpp"

using geopm::Policy;
using geopm::Exception;

::testing::AssertionResult
PoliciesAreSame(const Policy &p1, const Policy &p2)
{
    if (p1 != p2) {
        return ::testing::AssertionFailure() << "{" << p1 << "} is not equivalent to {" << p2 << "}";
    }
    return ::testing::AssertionSuccess();
}

::testing::AssertionResult
PoliciesAreNotSame(const Policy &p1, const Policy &p2)
{
    if (p1 == p2) {
        return ::testing::AssertionFailure() << "{" << p1 << "} is equivalent to {" << p2 << "}";
    }
    return ::testing::AssertionSuccess();
}

TEST(PolicyTest, construct)
{
    Policy pol1;
    EXPECT_EQ(0, pol1.size());

    std::vector<double> values {4.4, 5.5};
    Policy pol2 {values};
    EXPECT_EQ(2, pol2.size());
    EXPECT_EQ(5.5, pol2[1]);
    Policy pol2a {4.4, 5.5};

    Policy pol3 {pol2};
    EXPECT_EQ(2, pol3.size());
    EXPECT_EQ(4.4, pol3[0]);
    EXPECT_TRUE(pol2 == pol3);

    Policy pol4 = pol3;
    EXPECT_TRUE(pol3 == pol4);
    EXPECT_TRUE(pol1 != pol4);
}

TEST(PolicyTest, to_vector)
{
    std::vector<double> v1 {7.4, 6.33, 5.2};
    Policy p1 {v1};
    EXPECT_EQ(v1, p1.to_vector());
    std::vector<double> v2;
    Policy p2;
    EXPECT_EQ(v2, p2.to_vector());
}

TEST(PolicyTest, equality)
{
    EXPECT_TRUE(PoliciesAreSame({4, 5}, {4, 5}));
    EXPECT_TRUE(PoliciesAreSame({6, 5}, {6, 5, NAN}));
    EXPECT_TRUE(PoliciesAreSame({}, {NAN, NAN}));
    EXPECT_TRUE(PoliciesAreSame({8.8, NAN, NAN}, {8.8, NAN}));
    EXPECT_TRUE(PoliciesAreNotSame({5, 4, 3}, {NAN, 4, 3}));
    EXPECT_TRUE(PoliciesAreNotSame({8, 9, NAN}, {8, 9, 7}));
    EXPECT_TRUE(PoliciesAreNotSame({2, 7}, {2}));
}

TEST(PolicyTest, construct_from_agent_name)
{
    // instead of constructor, use factory function
}


TEST(PolicyTest, get_set_field)
{
    // requires agent name to get policy names

    // maybe also support indexing with [] (want .at() ?)

    // todo: check bounds in [] operator
}


TEST(PolicyTest, to_string)
{
    EXPECT_EQ("6.6", Policy({6.6}).to_string(", "));
    EXPECT_EQ("4,5,6", Policy({4, 5, 6}).to_string(","));
    EXPECT_EQ("NAN, NAN, 0", Policy({NAN, NAN, 0.0}).to_string(", "));
    EXPECT_EQ("8.8|7.7|6.6", Policy({8.8, 7.7, 6.6}).to_string("|"));
}

TEST(PolicyTest, to_json_string)
{
    // requires agent policy name to be passed

    // todo: use to replace geopm_agent_policy_json

    std::string json1 = "{}";
    EXPECT_EQ(json1, Policy({}).to_json({}));

    std::string json2 = "{\n"
                        "    \"val\": 5.5\n"
                        "}";
    EXPECT_EQ(json2, Policy({5.5}).to_json({"val"}));

    std::string json3 = "{\n"
                        "    \"banana\": 44.44,\n"
                        "    \"apple\": NAN,\n"
                        "    \"coconut\": 0,\n"
                        "    \"durian\": 8.76\n"
                        "}";
    EXPECT_EQ(json3, Policy({44.4, NAN, 0, 8.76}).to_json({"banana", "apple", "coconut", "durian"}));

    // errors:
    GEOPM_EXPECT_THROW_MESSAGE(Policy({1, 2, 3}).to_json({"one", "two"}),
                               GEOPM_ERROR_INVALID, "number of policy names");
    GEOPM_EXPECT_THROW_MESSAGE(Policy({1}).to_json({"one", "two"}),
                               GEOPM_ERROR_INVALID, "number of policy names");
}

TEST(PolicyTest, fill_nans)
{
    // fill larger
    // fill shrink with nans
    // shrink with values is an error
    // OR don't support shrinking??

    // {5.6, 7.8}
    // policy.resize(5)
    // {5.6, 7.8, nan, nan, nan}
}
