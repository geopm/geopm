/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#include "geopm_test.hpp"

using geopm::Policy;

TEST(PolicyTest, get_set_fields)
{
    Policy::PolicyField f0 {"weight", 4.4};
    Policy::PolicyField f1 {"size", 7};
    Policy pol({f0, f1});
    EXPECT_EQ(f0.default_value, pol[0]);
    EXPECT_EQ(f0.default_value, pol[f0.name]);
    EXPECT_EQ(f1.default_value, pol[1]);
    EXPECT_EQ(f1.default_value, pol[f1.name]);

    // set a new value by index
    pol[1] = 10.1;
    EXPECT_EQ(10.1, pol[1]);
    EXPECT_EQ(10.1, pol[f1.name]);

    // reset to default
    pol[1] = NAN;
    EXPECT_EQ(f1.default_value, pol[1]);
    EXPECT_EQ(f1.default_value, pol[f1.name]);

    // set a new value by name
    pol["weight"] = 6.5;
    EXPECT_EQ(6.5, pol[0]);
    EXPECT_EQ(6.5, pol[f0.name]);

    // reset to default
    pol["weight"] = NAN;
    EXPECT_EQ(f0.default_value, pol[0]);
    EXPECT_EQ(f0.default_value, pol[f0.name]);

    // out of bounds index error
    GEOPM_EXPECT_THROW_MESSAGE(pol[2], GEOPM_ERROR_INVALID,
                               "field index out of bounds");
    GEOPM_EXPECT_THROW_MESSAGE(pol[-1], GEOPM_ERROR_INVALID,
                               "field index out of bounds");

    // invalid name error
    GEOPM_EXPECT_THROW_MESSAGE(pol["invalid"], GEOPM_ERROR_INVALID,
                               "invalid policy field name");
}

TEST(PolicyTest, update_from_vector)
{
    Policy::PolicyField f0 {"red", 0.0};
    Policy::PolicyField f1 {"green", 1.0};
    Policy::PolicyField f2 {"blue", 0.4};
    Policy pol({f0, f1, f2});
    EXPECT_EQ({0.0, 1.0, 0.4}, pol.to_vector());

    std::vector<double> vals = {0.8, 0.2, 0.1};
    pol.update(vals);
    EXPECT_EQ(vals, pol.to_vector());

    // nan values reset to default
    vals = {0.5, NAN, NAN};
    pol.update(vals);
    EXPECT_EQ({0.5, 1.0, 0.4}, pol.to_vector());

    // errors: wrong size
    vals = {7, 7, 7, 7};
    GEOPM_EXPECT_THROW_MESSAGE(pol.update(vals));
}

TEST(PolicyTest, to_vector)
{
    Policy empty({});
    std::vector<double> result;
    EXPECT_EQ(result, empty.to_vector());

    Policy::PolicyField f0 {"waffle", 4.0};
    Policy::PolicyField f1 {"omelette", 6};
    Policy::PolicyField f2 {"muffin", 3.3};
    Policy::PolicyField f3 {"pancake", 200};
    Policy pol({f0, f1, f2, f3});

    std::vector<double> expected = {4.0, 6, 3.3, 200};
    EXPECT_EQ(expected, pol.to_vector());

    pol["muffin"] = 7.7;
    pol[3] = 300;
    expected = {4.0, 6, 7.7, 300};
    EXPECT_EQ(expected, pol.to_vector());

    pol["muffin"] = NAN;
    expected = {4.0, 6, 3.3, 300};
    EXPECT_EQ(expected, pol.to_vector());
}

TEST(PolicyTest, update_from_json)
{
    Policy::PolicyField f0 {"red", 0.0};
    Policy::PolicyField f1 {"green", 1.0};
    Policy::PolicyField f2 {"blue", 0.4};
    Policy pol({f0, f1, f2});

    pol.update("{\"red\": 8.88}");
    // union of existing and new values
    EXPECT_EQ({8.88, 1.0, 0.4}, pol.to_vector());


}

TEST(PolicyTest, to_json)
{
    Policy empty({});
    EXPECT_EQ("{}", empty.to_json());

    Policy::PolicyField f0 {"radar", 8.85};
    Policy::PolicyField f1 {"racecar", 54321};
    Policy::PolicyField f2 {"kayak", 10.1};
    Policy pol({f0, f1, f2});
    std::string expected = "{\"kayak\": 10.1, \"racecar\": 54321, \"radar\": 8.85}";
    EXPECT_EQ(expected, pol.to_json());

    pol["radar"] = 5.55;

    expected = "{\"kayak\": 10.1, \"racecar\": 54321, \"radar\": 5.55}";
    EXPECT_EQ(expected, pol.to_json());

    // TODO: no support for printing NAN
}
