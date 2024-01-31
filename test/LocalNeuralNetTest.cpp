/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

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
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

class LocalNeuralNetTest : public ::testing::Test
{
    protected:
        void SetUp() override;
	std::shared_ptr<MockTensorMath> m_fake_math;
	std::shared_ptr<MockDenseLayer> m_fake_layer1, m_fake_layer2;
	TensorOneD m_inp2, m_inp3, m_inp4, m_inp4s;
};

void LocalNeuralNetTest::SetUp()
{
    m_fake_math = std::make_shared<MockTensorMath>();

    m_inp2 = TensorOneD({1, 2}, m_fake_math);
    m_inp3 = TensorOneD({1, 2, 3}, m_fake_math);
    m_inp4 = TensorOneD({1, 2, 3, 4}, m_fake_math);
    m_inp4s = TensorOneD({4, 0, 3, 1}, m_fake_math);

    m_fake_layer1 = std::make_shared<MockDenseLayer>();
    m_fake_layer2 = std::make_shared<MockDenseLayer>();

    EXPECT_CALL(*m_fake_layer1, get_input_dim()).WillRepeatedly(Return(2u));
    EXPECT_CALL(*m_fake_layer1, get_output_dim()).WillRepeatedly(Return(4u));
    EXPECT_CALL(*m_fake_layer2, get_input_dim()).WillRepeatedly(Return(4u));
    EXPECT_CALL(*m_fake_layer2, get_output_dim()).WillRepeatedly(Return(3u));
}

TEST_F(LocalNeuralNetTest, test_inference)
{
    LocalNeuralNetImp net({m_fake_layer1, m_fake_layer2});

    EXPECT_CALL(*m_fake_layer1, forward(TensorOneDEqualTo(m_inp2))).WillOnce(Return(m_inp4));
    EXPECT_CALL(*m_fake_layer2, forward(TensorOneDEqualTo(m_inp4s))).WillOnce(Return(m_inp3));

    EXPECT_CALL(*m_fake_math, sigmoid(TensorOneDEqualTo(m_inp4))).WillOnce(Return(m_inp4s));

    EXPECT_THAT(net.forward(m_inp2), TensorOneDEqualTo(m_inp3));
}

TEST_F(LocalNeuralNetTest, test_bad_dimensions)
{
    {
        GEOPM_EXPECT_THROW_MESSAGE(LocalNeuralNetImp({}),
                                   GEOPM_ERROR_INVALID,
                                   "Empty layers");
    }

    {
        GEOPM_EXPECT_THROW_MESSAGE(LocalNeuralNetImp({m_fake_layer2, m_fake_layer1}),
                                   GEOPM_ERROR_INVALID,
                                   "Incompatible dimensions for consecutive layers.");
    }

    {
        LocalNeuralNetImp net({m_fake_layer1, m_fake_layer2});

        GEOPM_EXPECT_THROW_MESSAGE(net.forward(m_inp4),
                                   GEOPM_ERROR_INVALID,
                                   "Input vector dimension is incompatible");
    }
}
