/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "MockTensorMath.hpp"
#include "TensorOneDMatcher.hpp"
#include "TensorTwoDMatcher.hpp"

using geopm::TensorOneD;
using geopm::TensorTwoD;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

class TensorTwoDTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorTwoD mat;
};

void TensorTwoDTest::SetUp()
{
    mat.set_dim(2, 3);

    mat[0][0] = 1;
    mat[0][1] = 2;
    mat[0][2] = 3;
    mat[1][0] = 4;
    mat[1][1] = 5;
    mat[1][2] = 6;
}

TEST_F(TensorTwoDTest, test_vector_product)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<std::vector<float> > mat_a = {{1, 2}, {3, 4}, {5, 6}};
    std::vector<float> vec_b = {7, 8};
    std::vector<float> vec_c = {9, 10, 11};

    TensorTwoD tensor_a(mat_a, fake_math);
    TensorOneD tensor_b(vec_b, fake_math);
    TensorOneD tensor_c(vec_c, fake_math);

    EXPECT_CALL(*fake_math, multiply(TensorTwoDEqualTo(tensor_a), TensorOneDEqualTo(tensor_b))).WillOnce(Return(tensor_c));
    TensorOneD tensor_d = tensor_a * tensor_b;

    EXPECT_EQ(tensor_c.get_data(), tensor_d.get_data());
}

TEST_F(TensorTwoDTest, test_copy_constructor) {
    TensorTwoD copy(mat);
    EXPECT_EQ(1, copy[0][0]);
    EXPECT_EQ(2, copy[0][1]);
    EXPECT_EQ(3, copy[0][2]);
    EXPECT_EQ(4, copy[1][0]);
    EXPECT_EQ(5, copy[1][1]);
    EXPECT_EQ(6, copy[1][2]);

    // check that the copy is deep
    copy[1][0] = -1;
    EXPECT_EQ(4, mat[1][0]);
    EXPECT_EQ(-1, copy[1][0]);
}

TEST_F(TensorTwoDTest, test_array_overload) {
    const TensorTwoD mat_copy(mat);
    mat[0] = mat_copy[1];
    EXPECT_EQ(4, mat[0][0]);
    EXPECT_EQ(5, mat[0][1]);
    EXPECT_EQ(6, mat[0][2]);

    // check that the copy is deep
    mat[0][0] = 7;
    EXPECT_EQ(7, mat[0][0]);
    EXPECT_EQ(4, mat_copy[1][0]);
}

TEST_F(TensorTwoDTest, test_input) {
    std::vector<std::vector<float> > vals = {{1}, {2}};
    TensorTwoD x;
    x = TensorTwoD(vals);
    EXPECT_EQ(2u, x.get_rows());
    EXPECT_EQ(1u, x.get_cols());
    EXPECT_EQ(1, x[0][0]);
    EXPECT_EQ(2, x[1][0]);
}

TEST_F(TensorTwoDTest, test_degenerate_size) {
    TensorTwoD x;
    EXPECT_EQ(0u, x.get_cols());
}

TEST_F(TensorTwoDTest, test_copy) {
    TensorTwoD copy(3, 4);
    copy.set_dim(1, 1);
    copy = mat;

    // copy is successful
    EXPECT_EQ(1, copy[0][0]);
    EXPECT_EQ(2, copy[0][1]);
    EXPECT_EQ(3, copy[0][2]);
    EXPECT_EQ(4, copy[1][0]);
    EXPECT_EQ(5, copy[1][1]);
    EXPECT_EQ(6, copy[1][2]);

    // check that the copy is deep
    copy[1][0] = -1;
    EXPECT_EQ(4, mat[1][0]);
    EXPECT_EQ(-1, copy[1][0]);
}

TEST_F(TensorTwoDTest, test_bad_dimensions)
{
    std::vector<std::vector<float> > vals = {{1}, {2, 3}};
    EXPECT_THROW(new TensorTwoD(vals), geopm::Exception);  // TODO - new?
}

TEST_F(TensorTwoDTest, test_empty_weights)
{
    std::vector<std::vector<float> > vals = {};
    EXPECT_THROW(new TensorTwoD(vals), geopm::Exception);  // TODO - new?
}
