/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorTwoD.hpp"

#include <iostream>

using geopm::TensorOneD;
using geopm::TensorTwoD;

class TensorTwoDIntegrationTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorTwoD mat;
        TensorTwoD row;
};

void TensorTwoDIntegrationTest::SetUp()
{
    mat.set_dim(2, 3);

    mat[0][0] = 1;
    mat[0][1] = 2;
    mat[0][2] = 3;
    mat[1][0] = 4;
    mat[1][1] = 5;
    mat[1][2] = 6;

    row.set_dim(1, 3);
    row[0][0] = 1;
    row[0][1] = 2;
    row[0][2] = 3;
}

TEST_F(TensorTwoDIntegrationTest, test_mat_prod) {
    TensorOneD prod = mat * row[0];
    EXPECT_EQ(2u, prod.get_dim());
    EXPECT_EQ(14, prod[0]);
    EXPECT_EQ(32, prod[1]);
}

TEST_F(TensorTwoDIntegrationTest, test_bad_dimensions) {
    row.set_dim(1, 2);
    EXPECT_THROW(mat * row[0], geopm::Exception);
    EXPECT_THROW(row.set_dim(0, 1), geopm::Exception);
}
