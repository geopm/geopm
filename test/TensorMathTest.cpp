/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <cmath>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

using geopm::TensorMathImp;
using geopm::TensorOneD;
using geopm::TensorTwoD;

class TensorMathTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorOneD m_one, m_two, three;
        TensorTwoD m_mat, m_row;
        TensorMathImp m_math;
};

void TensorMathTest::SetUp()
{
    m_one.set_dim(2);
    m_two.set_dim(2);
    three.set_dim(3);

    m_one[0] = 1;
    m_one[1] = 2;
    m_two[0] = 3;
    m_two[1] = 4;
    three[0] = 0;
    three[1] = 1;
    three[2] = 1;

    m_mat.set_dim(2, 3);

    m_mat[0][0] = 1;
    m_mat[0][1] = 2;
    m_mat[0][2] = 3;
    m_mat[1][0] = 4;
    m_mat[1][1] = 5;
    m_mat[1][2] = 6;

    m_row.set_dim(1, 3);
    m_row[0][0] = 1;
    m_row[0][1] = 2;
    m_row[0][2] = 3;
}

TEST_F(TensorMathTest, test_sum)
{
    TensorOneD four = m_math.add(m_one, m_two);
    EXPECT_EQ(4, four[0]);
    EXPECT_EQ(6, four[1]);
}

TEST_F(TensorMathTest, test_self_sum)
{
    TensorOneD four = m_math.add(m_two, m_two);
    EXPECT_EQ(6, four[0]);
    EXPECT_EQ(8, four[1]);
}

TEST_F(TensorMathTest, test_diff)
{
    TensorOneD four = m_math.subtract(m_one, m_two);
    EXPECT_EQ(-2, four[0]);
    EXPECT_EQ(-2, four[1]);
}

TEST_F(TensorMathTest, test_self_diff)
{
    TensorOneD four = m_math.subtract(m_one, m_one);
    EXPECT_EQ(0, four[0]);
    EXPECT_EQ(0, four[1]);
}

TEST_F(TensorMathTest, test_dot)
{
    EXPECT_EQ(11, m_math.inner_product(m_one, m_two));
}

TEST_F(TensorMathTest, test_sigmoid)
{
    TensorOneD activations(5), boundary_act(2);

    activations[0] = -log(1/0.1 - 1);
    activations[1] = -log(1/0.25 - 1);
    activations[2] = -log(1/0.5 - 1);
    activations[3] = -log(1/0.75 - 1);
    activations[4] = -log(1/0.9 - 1);


    TensorOneD output = m_math.sigmoid(activations);

    EXPECT_DOUBLE_EQ(0.1, output[0]);
    EXPECT_DOUBLE_EQ(0.25, output[1]);
    EXPECT_DOUBLE_EQ(0.5, output[2]);
    EXPECT_DOUBLE_EQ(0.75, output[3]);
    EXPECT_DOUBLE_EQ(0.9, output[4]);

    boundary_act[0] = -HUGE_VAL;
    boundary_act[1] = HUGE_VAL;

    TensorOneD boundary_out = m_math.sigmoid(boundary_act);

    EXPECT_DOUBLE_EQ(0.0, boundary_out[0]);
    EXPECT_DOUBLE_EQ(1.0, boundary_out[1]);
}

TEST_F(TensorMathTest, test_mat_prod)
{
    TensorOneD prod = m_math.multiply(m_mat, m_row[0]);
    EXPECT_EQ(2u, prod.get_dim());
    EXPECT_EQ(14, prod[0]);
    EXPECT_EQ(32, prod[1]);
}

TEST_F(TensorMathTest, test_bad_dimensions)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_math.add(m_one, three), GEOPM_ERROR_INVALID, "mismatched dimensions");
    GEOPM_EXPECT_THROW_MESSAGE(m_math.subtract(m_one, three), GEOPM_ERROR_INVALID, "mismatched dimensions");
    GEOPM_EXPECT_THROW_MESSAGE(m_math.inner_product(m_one, three), GEOPM_ERROR_INVALID, "mismatched dimensions");
    m_row.set_dim(1, 2);
    GEOPM_EXPECT_THROW_MESSAGE(m_math.multiply(m_mat, m_row[0]), GEOPM_ERROR_INVALID, "incompatible dimensions");
}
