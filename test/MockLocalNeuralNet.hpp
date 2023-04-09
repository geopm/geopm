/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKLOCALNEURALNET_HPP_INCLUDE
#define MOCKLOCALNEURALNET_HPP_INCLUDE

#include "gmock/gmock.h"
#include "LocalNeuralNet.hpp"

namespace geopm {

class MockLocalNeuralNet : public LocalNeuralNet {
 public:
  MOCK_METHOD(TensorOneD, forward, (const TensorOneD &input), (const override));
  MOCK_METHOD(size_t, get_input_dim, (), (const override));
  MOCK_METHOD(size_t, get_output_dim, (), (const override));
};

}  // namespace geopm

#endif //MOCKLOCALNEURALNET_HPP_INCLUDE
