/*
 * Copyright (c) 2015 - 2024, Intel Corporation
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
    /// @brief Class to generate objects related to feed-forward neural nets,
    ///        i.e. TensorOneD, TensorTwoD, DenseLayer, and LocalNeuralNet objects.
    class NNFactory {
        public:
            /// @brief Returns a unique pointer to a concrete object constructed
            ///        using the underlying implementation.
            static std::unique_ptr<NNFactory> make_unique();
            /// @brief Returns a shared pointer to a concrete object constructed
            ///        using the underlying implementation.
            static std::shared_ptr<NNFactory> make_shared();
            virtual ~NNFactory() = default;
            /// @brief Create a LocalNeuralNet
            ///
            /// @param [in] layers A shared pointer to a vector of DenseLayers
            ///
            /// @return Returns a shared pointer to a LocalNeuralNet instance
            virtual std::shared_ptr<LocalNeuralNet> createLocalNeuralNet(const std::vector<std::shared_ptr<DenseLayer> > &layers) const = 0;
            /// @brief Create a Dense Layer
            ///
            /// @param [in] weights TensorTwoD instance (matrix)
            ///
            /// @param [in] biases TensorOneD instance (vector)
            ///
            /// @return Returns a shared pointer to the DenseLayer instance
            virtual std::shared_ptr<DenseLayer> createDenseLayer(const TensorTwoD &weights, const TensorOneD &biases) const = 0;
            /// @brief Create a TensorTwoD object.
            ///
            /// @param [in] vals Matrix doubles to fill TensorTwoD object
            ///
            /// @return Returns a TensorTwoD instance
            virtual TensorTwoD createTensorTwoD(const std::vector<std::vector<double> > &vals) const = 0;
            /// @brief Create a TensorOneD object.
            ///
            /// @param [in] vals Vector of doubles to fill TensorOneD object
            ///
            /// @return Returns a TensorOneD instance
            virtual TensorOneD createTensorOneD(const std::vector<double> &vals) const = 0;
    };
}

#endif  // NNFACTORY_HPP_INCLUDE
