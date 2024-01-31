/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

#include "DenseLayer.hpp"
#include "DenseLayerImp.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "MockTensorMath.hpp"
#include "TensorOneDMatcher.hpp"
#include "TensorTwoDMatcher.hpp"

using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

using geopm::TensorOneD;
using geopm::TensorTwoD;
using geopm::DenseLayer;
using geopm::DenseLayerImp;

class DenseLayerTest : public ::testing::Test
{
    protected:
        void SetUp() override;
        std::shared_ptr<MockTensorMath> m_fake_math;
        TensorTwoD m_weights;
        TensorOneD m_biases, m_tmp1, m_tmp2, m_inp3, m_inp4;
};

void DenseLayerTest::SetUp()
{
    m_fake_math = std::make_shared<MockTensorMath>();
    m_weights = TensorTwoD({{1, 2, 3}, {4, 5, 6}}, m_fake_math);
    m_biases = TensorOneD({7, 8}, m_fake_math);
    m_tmp1 = TensorOneD({3, 8, 9, 30}, m_fake_math);
    m_tmp2 = TensorOneD({10, 8, -1}, m_fake_math);
    m_inp3 = TensorOneD({1, 2, 3}, m_fake_math);
    m_inp4 = TensorOneD({1, 2, 3, 4}, m_fake_math);
}

TEST_F(DenseLayerTest, test_inference) {
    DenseLayerImp layer(m_weights, m_biases);

    EXPECT_CALL(*m_fake_math,
            multiply(TensorTwoDEqualTo(m_weights),
                TensorOneDEqualTo(m_inp3)))
        .WillOnce(Return(m_tmp1));
    EXPECT_CALL(*m_fake_math,
            add(TensorOneDEqualTo(m_biases),
                TensorOneDEqualTo(m_tmp1)))
        .WillOnce(Return(m_tmp2));

    EXPECT_EQ(3u, layer.get_input_dim());
    EXPECT_EQ(2u, layer.get_output_dim());

    EXPECT_THAT(layer.forward(m_inp3), TensorOneDEqualTo(m_tmp2));
}

TEST_F(DenseLayerTest, test_bad_dimensions) {
    DenseLayerImp layer(m_weights, m_biases);

    GEOPM_EXPECT_THROW_MESSAGE(DenseLayerImp(TensorTwoD(), m_inp3),
                               GEOPM_ERROR_INVALID,
                               "Empty array is invalid for neural network weights.");

    GEOPM_EXPECT_THROW_MESSAGE(DenseLayerImp(m_weights, m_inp4),
                               GEOPM_ERROR_INVALID,
                               "Incompatible dimensions for weights and biases.");

    GEOPM_EXPECT_THROW_MESSAGE(layer.forward(m_inp4),
                               GEOPM_ERROR_INVALID,
                               "Input vector dimension is incompatible with network");
}
