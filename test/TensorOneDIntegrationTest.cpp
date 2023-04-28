/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

#include "TensorOneD.hpp"

using geopm::TensorOneD;

class TensorOneDIntegrationTest : public ::testing::Test
{
    protected:
        void SetUp();

        TensorOneD m_one, m_two, m_three;
};

void TensorOneDIntegrationTest::SetUp()
{
    m_one.set_dim(2);
    m_two.set_dim(2);
    m_three.set_dim(3);

    m_one[0] = 1;
    m_one[1] = 2;
    m_two[0] = 3;
    m_two[1] = 4;
    m_three[0] = 0;
    m_three[1] = 1;
    m_three[2] = 1;
}

TEST_F(TensorOneDIntegrationTest, test_sum)
{
    TensorOneD four = m_one + m_two;
    EXPECT_EQ(4, four[0]);
    EXPECT_EQ(6, four[1]);
}

TEST_F(TensorOneDIntegrationTest, test_self_sum)
{
    TensorOneD four = m_two + m_two;
    EXPECT_EQ(6, four[0]);
    EXPECT_EQ(8, four[1]);
}

TEST_F(TensorOneDIntegrationTest, test_diff)
{
    TensorOneD four(m_one - m_two);
    EXPECT_EQ(-2, four[0]);
    EXPECT_EQ(-2, four[1]);
}

TEST_F(TensorOneDIntegrationTest, test_self_diff)
{
    TensorOneD four = m_one - m_one;
    EXPECT_EQ(0, four[0]);
    EXPECT_EQ(0, four[1]);
}

TEST_F(TensorOneDIntegrationTest, test_dot)
{
    EXPECT_EQ(11, m_one * m_two);
}

TEST_F(TensorOneDIntegrationTest, test_sigmoid)
{
    TensorOneD activations(5);

    activations[0] = -log(1/0.1 - 1);
    activations[1] = -log(1/0.25 - 1);
    activations[2] = -log(1/0.5 - 1);
    activations[3] = -log(1/0.75 - 1);
    activations[4] = -log(1/0.9 - 1);

    TensorOneD output = activations.sigmoid();

    EXPECT_FLOAT_EQ(0.1, output[0]);
    EXPECT_FLOAT_EQ(0.25, output[1]);
    EXPECT_FLOAT_EQ(0.5, output[2]);
    EXPECT_FLOAT_EQ(0.75, output[3]);
    EXPECT_FLOAT_EQ(0.9, output[4]);
}

TEST_F(TensorOneDIntegrationTest, test_copy)
{
    m_two = m_one;

    // copy is successful
    EXPECT_EQ(1, m_two[0]);
    EXPECT_EQ(2, m_two[1]);

    // copy is deep
    m_two[0] = 9;
    EXPECT_EQ(1, m_one[0]);
    EXPECT_EQ(9, m_two[0]);
}

TEST_F(TensorOneDIntegrationTest, test_input)
{
    TensorOneD x(3);
    x.set_dim(4);
    std::vector<double> vals = {8, 16};
    x = TensorOneD(vals);
    EXPECT_EQ(2u, x.get_dim());
    EXPECT_EQ(8, x[0]);
    EXPECT_EQ(16, x[1]);
}

TEST_F(TensorOneDIntegrationTest, test_bad_dimensions)
{
    GEOPM_EXPECT_THROW_MESSAGE(m_one + m_three, GEOPM_ERROR_INVALID, "mismatched dimensions");
    GEOPM_EXPECT_THROW_MESSAGE(m_one - m_three, GEOPM_ERROR_INVALID, "mismatched dimensions");
    GEOPM_EXPECT_THROW_MESSAGE(m_one * m_three, GEOPM_ERROR_INVALID, "mismatched dimensions");
}
