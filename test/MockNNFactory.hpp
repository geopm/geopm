/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKNNFACTORY_HPP_INCLUDE
#define MOCKNNFACTORY_HPP_INCLUDE

#include "gmock/gmock.h"
#include "NNFactory.hpp"

namespace geopm {

class MockNNFactory : public NNFactory {
 public:
  MOCK_METHOD(std::shared_ptr<LocalNeuralNet>, createLocalNeuralNet, (const std::vector<std::shared_ptr<DenseLayer>> &layers), (const, override));
  MOCK_METHOD(std::shared_ptr<DenseLayer>, createDenseLayer, (const TensorTwoD &weights, const TensorOneD &biases), (const, override));
  MOCK_METHOD(TensorTwoD, createTensorTwoD, (const std::vector<std::vector<double>> &vals), (const, override));
  MOCK_METHOD(TensorOneD, createTensorOneD, (const std::vector<double> &vals), (const, override));
};

}  // namespace geopm

#endif // MOCKNNFACTORY_HPP_INCLUDE
