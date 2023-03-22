/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LOCALNEURALNET_HPP_INCLUDE
#define LOCALNEURALNET_HPP_INCLUDE

#include "DenseLayer.hpp"
#include "TensorOneD.hpp"

namespace geopm
{
    class TensorTwoD;

    ///  @brief Class to manage data and operations of feed forward neural nets
    ///         required for neural net inference.

    class LocalNeuralNet
    {
        public:
            virtual ~LocalNeuralNet() = default;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            ///        from a vector of DenseLayers
            /// 
            /// @param [in] Vector of DenseLayers
            static std::unique_ptr<LocalNeuralNet> make_unique(std::vector<std::shared_ptr<DenseLayer> > layers);
            /// @brief Perform inference using the instance weights and biases.
            /// 
            /// @param [in] TensorOneD vector of input signals.
            ///
            /// @return Returns a TensorOneD vector of output values.
            ///
            /// @throws geopm::Exception if input dimension is incompatible
            /// with network.
            virtual TensorOneD forward(const TensorOneD &inp) const = 0;

            TensorOneD operator()(const TensorOneD &input) const {
                return forward(input);
            }
    };
}

#endif  /* LOCALNEURALNET_HPP_INCLUDE */
