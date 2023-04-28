/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"

#include "TensorTwoD.hpp"
#include "geopm_test.hpp"

#include <iostream>

using geopm::TensorOneD;
using geopm::TensorTwoD;
using testing::Throw;

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
    GEOPM_EXPECT_THROW_MESSAGE(mat * row[0], GEOPM_ERROR_INVALID,
                               "Attempted to multiply matrix and vector with incompatible dimensions.");
    GEOPM_EXPECT_THROW_MESSAGE(row.set_dim(0, 1), GEOPM_ERROR_INVALID,
                               "Tried to allocate degenerate matrix.");
}
