/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKNNFACTORY_HPP_INCLUDE
#define MOCKNNFACTORY_HPP_INCLUDE

#include "gmock/gmock.h"
#include "NNFactory.hpp"

class MockNNFactory : public geopm::NNFactory
{
    public:
        MOCK_METHOD(std::shared_ptr<geopm::LocalNeuralNet>, createLocalNeuralNet,
                    (const std::vector<std::shared_ptr<geopm::DenseLayer>> &layers),
                    (const, override));
        MOCK_METHOD(std::shared_ptr<geopm::DenseLayer>, createDenseLayer,
                    (const geopm::TensorTwoD &weights, const geopm::TensorOneD &biases),
                    (const, override));
        MOCK_METHOD(geopm::TensorTwoD, createTensorTwoD,
                    (const std::vector<std::vector<double>> &vals),
                    (const, override));
        MOCK_METHOD(geopm::TensorOneD, createTensorOneD, (const std::vector<double> &vals),
                    (const, override));
};

#endif // MOCKNNFACTORY_HPP_INCLUDE
