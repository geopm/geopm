/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorOneD.hpp"

#include "MockTensorMath.hpp"
#include "TensorOneDMatcher.hpp"

using geopm::TensorOneD;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

TEST(TensorOneDTest, test_copy)
{
    TensorOneD one(std::vector<double>({1, 2})), two;

    two = one;

    // copy is successful
    EXPECT_EQ(1, two[0]);
    EXPECT_EQ(2, two[1]);

    // copy is deep
    two[0] = 9;
    EXPECT_EQ(1, one[0]);
    EXPECT_EQ(9, two[0]);
}

TEST(TensorOneDTest, test_diff)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<double> vec_a = {1, 2, 3};
    std::vector<double> vec_b = {6, 7};

    TensorOneD tensor_a(vec_a, fake_math);
    TensorOneD tensor_b(vec_b);

    ON_CALL(*fake_math, subtract(_, _)).WillByDefault(Return(tensor_b));
    EXPECT_CALL(*fake_math, subtract(TensorOneDEqualTo(tensor_a), TensorOneDEqualTo(tensor_a))).Times(1);
    TensorOneD tensor_c = tensor_a - tensor_a;

    EXPECT_EQ(tensor_b.get_data(), tensor_c.get_data());
}

TEST(TensorOneDTest, test_input)
{
    TensorOneD x(3);
    x.set_dim(4);
    std::vector<double> vals = {8, 16};
    x = TensorOneD(vals);
    EXPECT_EQ(2u, x.get_dim());
    EXPECT_EQ(8, x[0]);
    EXPECT_EQ(16, x[1]);
}

TEST(TensorOneDTest, test_equivalent)
{
    std::vector<double> vec_a = {1, 2, 3};
    std::vector<double> vec_b = {6, 7};

    TensorOneD tensor_a(vec_a);
    TensorOneD tensor_b(vec_b);

    EXPECT_TRUE(tensor_a == tensor_a);
    EXPECT_FALSE(tensor_a == tensor_b);
}

TEST(TensorOneDTest, test_prod)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<double> vec_a = {1, 2, 3};
    std::vector<double> vec_b = {6, 7};
    double retval = 5.0;

    TensorOneD tensor_a(vec_a, fake_math);
    TensorOneD tensor_b(vec_b);

    ON_CALL(*fake_math, inner_product(_, _)).WillByDefault(Return(retval));
    EXPECT_CALL(*fake_math, inner_product(TensorOneDEqualTo(tensor_a), TensorOneDEqualTo(tensor_b)))
        .Times(1);
    double prod = tensor_a * tensor_b;

    EXPECT_EQ(retval, prod);
}

TEST(TensorOneDTest, test_sum)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<double> vec_a = {1, 2, 3};
    std::vector<double> vec_b = {6, 7};

    TensorOneD tensor_a(vec_a, fake_math);
    TensorOneD tensor_b(vec_b);

    ON_CALL(*fake_math, add(_, _)).WillByDefault(Return(tensor_b));
    EXPECT_CALL(*fake_math, add(TensorOneDEqualTo(tensor_a), TensorOneDEqualTo(tensor_a))).Times(1);
    TensorOneD tensor_c = tensor_a + tensor_a;

    EXPECT_EQ(tensor_b.get_data(), tensor_c.get_data());
}

TEST(TensorOneDTest, test_sigmoid)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<double> vec_a = {1, 2, 3};
    std::vector<double> vec_b = {6, 7};

    TensorOneD tensor_a(vec_a, fake_math);
    TensorOneD tensor_b(vec_b);

    ON_CALL(*fake_math, sigmoid(_)).WillByDefault(Return(tensor_b));
    EXPECT_CALL(*fake_math, sigmoid(TensorOneDEqualTo(tensor_a))).Times(1);
    TensorOneD tensor_c = tensor_a.sigmoid();

    EXPECT_EQ(tensor_b.get_data(), tensor_c.get_data());
}
