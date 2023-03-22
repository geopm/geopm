/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DENSELAYER_HPP_INCLUDE
#define DENSELAYER_HPP_INCLUDE

#include "TensorOneD.hpp"

namespace geopm
{
    class TensorTwoD;

    /// @brief Class to perform operations on 1D and 2D Tensors,
    ///        aka vectors and matrices, suitable for use in
    ///        feed-forward neural networks.
    ///  @brief Class to store dense layers
    class DenseLayer
    {
        public:
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            ///        from an pair of weights and biases.
            /// 
            /// @param [in] weight matrix (TensorTwoD instance) and bias
            ///             vector (TensorOneD instance)
            /// 
            /// @throws geopm::Exception if dimensions are incompatible.
            static std::unique_ptr<DenseLayer> make_unique(const TensorTwoD &weights, const TensorOneD &biases);
            virtual ~DenseLayer() = default;
            /// @brief Perform inference using the instance weights and biases.
            /// 
            /// @param [in] TensorOneD vector of input signals.
            ///
            /// @return Returns a TensorOneD vector of output values.
            ///
            /// @throws geopm::Exception if input dimension is incompatible
            /// with the layer.
            virtual TensorOneD forward(const TensorOneD &input) const = 0;

            /// TODO docstrings
            virtual size_t get_input_dim() const = 0;
            virtual size_t get_output_dim() const = 0;

            TensorOneD operator()(const TensorOneD &input) const {
                return forward(input);
            }
    };
}

#endif /* DENSELAYER_HPP_INCLUDE */
