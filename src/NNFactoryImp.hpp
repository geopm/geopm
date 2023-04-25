/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef NNFACTORYIMP_HPP_INCLUDE
#define NNFACTORYIMP_HPP_INCLUDE

#include "NNFactory.hpp"

namespace geopm
{
    class NNFactoryImp : public NNFactory {
        public:
            NNFactoryImp() = default;
            virtual ~NNFactoryImp() = default;

            std::shared_ptr<LocalNeuralNet> createLocalNeuralNet(
                    const std::vector<std::shared_ptr<DenseLayer> > &layers) const override;
            std::shared_ptr<DenseLayer> createDenseLayer(const TensorTwoD &weights,
                                                         const TensorOneD &biases) const override;
            TensorTwoD createTensorTwoD(const std::vector<std::vector<double> > &vals)
                    const override;
            TensorOneD createTensorOneD(const std::vector<double> &vals) const override;
    };
}

#endif  // NNFACTORYIMP_HPP_INCLUDE
