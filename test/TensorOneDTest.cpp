/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorOneD.hpp"

#include "MockTensorMath.hpp"

using geopm::TensorOneD;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

class TensorOneDTest : public ::testing::Test
{
};

TEST_F(TensorOneDTest, test_sum)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<float> vec_a = {1, 2, 3};
    std::vector<float> vec_b = {6, 7};

    TensorOneD tensor_a(vec_a, fake_math);
    TensorOneD tensor_b(vec_b);

    //Mock::AllowLeak(&*fake_math);
    ON_CALL(*fake_math, add(_, _)).WillByDefault(Return(tensor_b));
    EXPECT_CALL(*fake_math, add(tensor_a, tensor_a)).Times(1);
    TensorOneD tensor_c = tensor_a + tensor_a;

    EXPECT_EQ(tensor_b.get_data(), tensor_c.get_data());
}

TEST_F(TensorOneDTest, test_copy)
{
    TensorOneD one(std::vector<float>({1, 2})), two;

    two = one;

    // copy is successful
    EXPECT_EQ(1, two[0]);
    EXPECT_EQ(2, two[1]);

    // copy is deep
    two[0] = 9;
    EXPECT_EQ(1, one[0]);
    EXPECT_EQ(9, two[0]);
}

TEST_F(TensorOneDTest, test_input)
{
    TensorOneD x(3);
    x.set_dim(4);
    std::vector<float> vals = {8, 16};
    x = TensorOneD(vals);
    EXPECT_EQ(2u, x.get_dim());
    EXPECT_EQ(8, x[0]);
    EXPECT_EQ(16, x[1]);
}
