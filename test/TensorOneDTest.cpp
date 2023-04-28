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

class TensorOneDTest : public ::testing::Test
{
    protected:
        void SetUp();
        std::vector<double> m_vec_a;
        TensorOneD m_tensor_b;
        std::shared_ptr<MockTensorMath> m_fake_math;
};

void TensorOneDTest::SetUp()
{
    m_vec_a.push_back(1);
    m_vec_a.push_back(2);
    m_vec_a.push_back(3);

    m_tensor_b.set_dim(2);
    m_tensor_b[0] = 6;
    m_tensor_b[1] = 7;

    m_fake_math = std::make_shared<MockTensorMath>();
}

TEST_F(TensorOneDTest, test_copy)
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

    TensorOneD three(two);

    // copy is successful
    EXPECT_EQ(9, three[0]);
    EXPECT_EQ(2, three[1]);

    // copy is deep
    three[0] = 4;
    EXPECT_EQ(1, one[0]);
    EXPECT_EQ(9, two[0]);
    EXPECT_EQ(4, three[0]);
}

TEST_F(TensorOneDTest, test_diff)
{
    TensorOneD tensor_a(m_vec_a, m_fake_math);

    ON_CALL(*m_fake_math, subtract(_, _)).WillByDefault(Return(m_tensor_b));
    EXPECT_CALL(*m_fake_math, subtract(TensorOneDEqualTo(tensor_a), TensorOneDEqualTo(tensor_a))).Times(1);
    TensorOneD tensor_c = tensor_a - tensor_a;

    EXPECT_EQ(m_tensor_b.get_data(), tensor_c.get_data());
}

TEST_F(TensorOneDTest, test_input)
{
    TensorOneD x(3);
    EXPECT_EQ(3u, x.get_dim());
    x.set_dim(4);
    EXPECT_EQ(4u, x.get_dim());
    std::vector<double> vals = {8, 16};
    x = TensorOneD(vals);
    EXPECT_EQ(2u, x.get_dim());
    EXPECT_EQ(8, x[0]);
    EXPECT_EQ(16, x[1]);
}

TEST_F(TensorOneDTest, test_equivalent)
{
    TensorOneD tensor_a(m_vec_a);

    EXPECT_TRUE(tensor_a == tensor_a);
    EXPECT_FALSE(tensor_a == m_tensor_b);
}

TEST_F(TensorOneDTest, test_prod)
{
    double retval = 5.0;
    TensorOneD tensor_a(m_vec_a, m_fake_math);

    ON_CALL(*m_fake_math, inner_product(_, _)).WillByDefault(Return(retval));
    EXPECT_CALL(*m_fake_math, inner_product(TensorOneDEqualTo(tensor_a), TensorOneDEqualTo(m_tensor_b)))
        .Times(1);
    double prod = tensor_a * m_tensor_b;

    EXPECT_EQ(retval, prod);
}

TEST_F(TensorOneDTest, test_sum)
{
    TensorOneD tensor_a(m_vec_a, m_fake_math);

    ON_CALL(*m_fake_math, add(_, _)).WillByDefault(Return(m_tensor_b));
    EXPECT_CALL(*m_fake_math, add(TensorOneDEqualTo(tensor_a), TensorOneDEqualTo(tensor_a))).Times(1);
    TensorOneD tensor_c = tensor_a + tensor_a;

    EXPECT_EQ(m_tensor_b.get_data(), tensor_c.get_data());
}

TEST_F(TensorOneDTest, test_sigmoid)
{
    TensorOneD tensor_a(m_vec_a, m_fake_math);

    ON_CALL(*m_fake_math, sigmoid(_)).WillByDefault(Return(m_tensor_b));
    EXPECT_CALL(*m_fake_math, sigmoid(TensorOneDEqualTo(tensor_a))).Times(1);
    TensorOneD tensor_c = tensor_a.sigmoid();

    EXPECT_EQ(m_tensor_b.get_data(), tensor_c.get_data());
}
