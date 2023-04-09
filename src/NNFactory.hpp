/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NNFACTORY_HPP_INCLUDE
#define NNFACTORY_HPP_INCLUDE

#include <memory>
#include <vector>

#include "DenseLayer.hpp"
#include "LocalNeuralNet.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

namespace geopm
{
    class NNFactory {
        public:
            static std::unique_ptr<NNFactory> make_unique();
            virtual ~NNFactory() = default;

            virtual std::shared_ptr<LocalNeuralNet> createLocalNeuralNet(const std::vector<std::shared_ptr<DenseLayer>> &layers) const = 0;
            virtual std::shared_ptr<DenseLayer> createDenseLayer(const TensorTwoD &weights, const TensorOneD &biases) const = 0;
            virtual TensorTwoD createTensorTwoD(const std::vector<std::vector<double>> &vals) const = 0;
            virtual TensorOneD createTensorOneD(const std::vector<double> &vals) const = 0;
    };
}

#endif  // NNFACTORY_HPP_INCLUDE
