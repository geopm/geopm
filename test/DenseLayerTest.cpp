/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

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

using geopm::TensorOneD;
using geopm::TensorTwoD;
using geopm::DenseLayer;
using geopm::DenseLayerImp;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

class DenseLayerTest : public ::testing::Test
{
    protected:
        void SetUp() override;
        std::shared_ptr<MockTensorMath> fake_math;
        TensorTwoD weights;
        TensorOneD biases, tmp1, tmp2, inp3, inp4;
};

void DenseLayerTest::SetUp()
{
    fake_math = std::make_shared<MockTensorMath>();
    weights = TensorTwoD({{1, 2, 3}, {4, 5, 6}}, fake_math);
    biases = TensorOneD({7, 8}, fake_math);
    tmp1 = TensorOneD({3, 8, 9, 30}, fake_math);
    tmp2 = TensorOneD({10, 8, -1}, fake_math);
    inp3 = TensorOneD({1, 2, 3}, fake_math);
    inp4 = TensorOneD({1, 2, 3, 4}, fake_math);
}

TEST_F(DenseLayerTest, test_inference) {
    DenseLayerImp layer(weights, biases);

    EXPECT_CALL(*fake_math,
            multiply(TensorTwoDEqualTo(weights),
                TensorOneDEqualTo(inp3)))
        .WillOnce(Return(tmp1));
    EXPECT_CALL(*fake_math,
            add(TensorOneDEqualTo(biases),
                TensorOneDEqualTo(tmp1)))
        .WillOnce(Return(tmp2));

    EXPECT_EQ(3u, layer.get_input_dim());
    EXPECT_EQ(2u, layer.get_output_dim());

    EXPECT_THAT(layer.forward(inp3), TensorOneDEqualTo(tmp2));
}

TEST_F(DenseLayerTest, test_bad_dimensions) {
    DenseLayerImp layer(weights, biases);

    GEOPM_EXPECT_THROW_MESSAGE(DenseLayerImp(TensorTwoD(), inp3),
                               GEOPM_ERROR_INVALID,
                               "Empty array is invalid for neural network weights.");

    GEOPM_EXPECT_THROW_MESSAGE(DenseLayerImp(weights, inp4),
                               GEOPM_ERROR_INVALID,
                               "Incompatible dimensions for weights and biases.");

    GEOPM_EXPECT_THROW_MESSAGE(layer.forward(inp4),
                               GEOPM_ERROR_INVALID,
                               "Input vector dimension is incompatible with network");
}
