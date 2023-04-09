/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "geopm/Exception.hpp"
#include "geopm_test.hpp"

#include "LocalNeuralNet.hpp"
#include "LocalNeuralNetImp.hpp"
#include "DenseLayer.hpp"
#include "TensorOneD.hpp"

#include "MockDenseLayer.hpp"
#include "MockTensorMath.hpp"
#include "TensorOneDMatcher.hpp"

using geopm::TensorOneD;
using geopm::DenseLayer;
using geopm::LocalNeuralNet;
using geopm::LocalNeuralNetImp;
using geopm::MockDenseLayer;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

class LocalNeuralNetTest : public ::testing::Test
{
    protected:
        void SetUp() override;
	std::shared_ptr<MockTensorMath> fake_math;
	std::shared_ptr<MockDenseLayer> fake_layer1, fake_layer2;
	TensorOneD inp2, inp3, inp4, inp4s;
};

void LocalNeuralNetTest::SetUp()
{
    fake_math = std::make_shared<MockTensorMath>();

    inp2 = TensorOneD({1, 2}, fake_math);
    inp3 = TensorOneD({1, 2, 3}, fake_math);
    inp4 = TensorOneD({1, 2, 3, 4}, fake_math);
    inp4s = TensorOneD({4, 0, 3, 1}, fake_math);

    fake_layer1 = std::make_shared<MockDenseLayer>();
    fake_layer2 = std::make_shared<MockDenseLayer>();

    EXPECT_CALL(*fake_layer1, get_input_dim()).WillRepeatedly(Return(2u));
    EXPECT_CALL(*fake_layer1, get_output_dim()).WillRepeatedly(Return(4u));
    EXPECT_CALL(*fake_layer2, get_input_dim()).WillRepeatedly(Return(4u));
    EXPECT_CALL(*fake_layer2, get_output_dim()).WillRepeatedly(Return(3u));
}

TEST_F(LocalNeuralNetTest, test_inference) {
    LocalNeuralNetImp net({fake_layer1, fake_layer2});

    EXPECT_CALL(*fake_layer1, forward(TensorOneDEqualTo(inp2))).WillOnce(Return(inp4));
    EXPECT_CALL(*fake_layer2, forward(TensorOneDEqualTo(inp4s))).WillOnce(Return(inp3));

    EXPECT_CALL(*fake_math, sigmoid(TensorOneDEqualTo(inp4))).WillOnce(Return(inp4s));

    EXPECT_THAT(net.forward(inp2), TensorOneDEqualTo(inp3));
}

TEST_F(LocalNeuralNetTest, test_bad_dimensions) {
    {
        GEOPM_EXPECT_THROW_MESSAGE(LocalNeuralNetImp({fake_layer2, fake_layer1}),
                                   GEOPM_ERROR_INVALID,
                                   "Incompatible dimensions for consecutive layers.");
    }

    {
        LocalNeuralNetImp net({fake_layer1, fake_layer2});

        GEOPM_EXPECT_THROW_MESSAGE(net.forward(inp4),
                                   GEOPM_ERROR_INVALID,
                                   "Input vector dimension is incompatible");
    }
}
