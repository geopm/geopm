/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <initializer_list>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "MockTensorMath.hpp"
#include "TensorOneDMatcher.hpp"
#include "TensorTwoDMatcher.hpp"
#include "geopm_test.hpp"

using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

using geopm::TensorOneD;
using geopm::TensorTwoD;
using testing::Throw;

class TensorTwoDTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorTwoD m_mat;
};

void TensorTwoDTest::SetUp()
{
    m_mat.set_dim(2, 3);

    m_mat[0][0] = 1;
    m_mat[0][1] = 2;
    m_mat[0][2] = 3;
    m_mat[1][0] = 4;
    m_mat[1][1] = 5;
    m_mat[1][2] = 6;
}

TEST_F(TensorTwoDTest, test_vector_product)
{
    auto fake_math = std::make_shared<MockTensorMath>();

    std::vector<std::vector<double> > mat_a = {{1, 2}, {3, 4}, {5, 6}};
    std::vector<double> vec_b = {7, 8};
    std::vector<double> vec_c = {9, 10, 11};

    TensorTwoD tensor_a(mat_a, fake_math);
    TensorOneD tensor_b(vec_b, fake_math);
    TensorOneD tensor_c(vec_c, fake_math);

    EXPECT_CALL(*fake_math, multiply(TensorTwoDEqualTo(tensor_a),
                TensorOneDEqualTo(tensor_b))).WillOnce(Return(tensor_c));
    TensorOneD tensor_d = tensor_a * tensor_b;

    EXPECT_EQ(tensor_c.get_data(), tensor_d.get_data());
}

TEST_F(TensorTwoDTest, test_copy_constructor) {
    TensorTwoD copy(m_mat);
    EXPECT_EQ(1, copy[0][0]);
    EXPECT_EQ(2, copy[0][1]);
    EXPECT_EQ(3, copy[0][2]);
    EXPECT_EQ(4, copy[1][0]);
    EXPECT_EQ(5, copy[1][1]);
    EXPECT_EQ(6, copy[1][2]);

    // check that the copy is deep
    copy[1][0] = -1;
    EXPECT_EQ(4, m_mat[1][0]);
    EXPECT_EQ(-1, copy[1][0]);
}

TEST_F(TensorTwoDTest, test_array_overload) {
    const TensorTwoD mat_copy(m_mat);
    m_mat[0] = mat_copy[1];
    EXPECT_EQ(4, m_mat[0][0]);
    EXPECT_EQ(5, m_mat[0][1]);
    EXPECT_EQ(6, m_mat[0][2]);

    // check that the copy is deep
    m_mat[0][0] = 7;
    EXPECT_EQ(7, m_mat[0][0]);
    EXPECT_EQ(4, mat_copy[1][0]);
}

TEST_F(TensorTwoDTest, test_input) {
    std::vector<std::vector<double> > vals = {{1}, {2}};
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
    EXPECT_EQ(0u, x.get_rows());
}

TEST_F(TensorTwoDTest, test_copy) {
    TensorTwoD copy(3, 4);
    copy.set_dim(1, 1);
    copy = m_mat;

    // copy is successful
    EXPECT_EQ(1, copy[0][0]);
    EXPECT_EQ(2, copy[0][1]);
    EXPECT_EQ(3, copy[0][2]);
    EXPECT_EQ(4, copy[1][0]);
    EXPECT_EQ(5, copy[1][1]);
    EXPECT_EQ(6, copy[1][2]);

    // check that the copy is deep
    copy[1][0] = -1;
    EXPECT_EQ(4, m_mat[1][0]);
    EXPECT_EQ(-1, copy[1][0]);
}

TEST_F(TensorTwoDTest, test_bad_dimensions)
{
    std::vector<std::vector<double> > vals = {{1}, {2, 3}};
    GEOPM_EXPECT_THROW_MESSAGE(new TensorTwoD(vals), GEOPM_ERROR_INVALID,
                               "Attempt to load non-rectangular matrix.");
}

TEST_F(TensorTwoDTest, test_empty_weights)
{
    std::vector<std::vector<double> > vals = {};
    GEOPM_EXPECT_THROW_MESSAGE(new TensorTwoD(vals), GEOPM_ERROR_INVALID,
                               "Empty array is invalid for neural network weights.");
}

TEST_F(TensorTwoDTest, test_set_data)
{
    TensorTwoD xx(2, 3);
    std::vector<TensorOneD> vals_bad = {TensorOneD(std::initializer_list<double>{1}),
                                        TensorOneD({2, 3})};
    std::vector<TensorOneD> vals_good = {TensorOneD({1, 4}), TensorOneD({2, 3})};
    GEOPM_EXPECT_THROW_MESSAGE(xx.set_data(vals_bad), GEOPM_ERROR_INVALID,
                               "Attempt to load non-rectangular matrix.");

    xx.set_data(vals_good);
    EXPECT_THAT(xx, TensorTwoDMatcher(vals_good));
}

TEST_F(TensorTwoDTest, test_equality)
{
    TensorTwoD xx({{1, 2}, {3, 4}});
    TensorTwoD yy({{1, 2}, {3, 4}});
    TensorTwoD zz({{1, 2}, {3, 4}, {5, 6}});
    EXPECT_EQ(true, xx == yy);
    EXPECT_EQ(false, xx == zz);
}
