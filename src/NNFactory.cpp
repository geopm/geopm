/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

#include "DenseLayerImp.hpp"
#include "LocalNeuralNetImp.hpp"
#include "NNFactoryImp.hpp"

namespace geopm {
    std::unique_ptr<NNFactory> NNFactory::make_unique()
    {
        return geopm::make_unique<NNFactoryImp>();
    }

    std::shared_ptr<NNFactory> NNFactory::make_shared()
    {
        return std::make_shared<NNFactoryImp>();
    }

    std::shared_ptr<LocalNeuralNet> NNFactoryImp::createLocalNeuralNet(
            const std::vector<std::shared_ptr<DenseLayer> > &layers) const
    {
        return std::make_shared<LocalNeuralNetImp>(layers);
    }

    std::shared_ptr<DenseLayer> NNFactoryImp::createDenseLayer(const TensorTwoD &weights,
                                                               const TensorOneD &biases) const
    {
        return std::make_shared<DenseLayerImp>(weights, biases);
    }

    TensorTwoD NNFactoryImp::createTensorTwoD(const std::vector<std::vector<double> > &vals) const
    {
        return TensorTwoD(vals);
    }

    TensorOneD NNFactoryImp::createTensorOneD(const std::vector<double> &vals) const
    {
        return TensorOneD(vals);
    }
}
