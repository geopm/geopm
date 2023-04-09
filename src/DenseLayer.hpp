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

    /// @brief Class to store dense layers and perform operations on 
    ///        the layers' 1D and 2D Tensors, aka vectors and matrices, 
    ///        suitable for use in feed-forward neural networks.
    class DenseLayer
    {
        public:
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            ///        from an pair of weights and biases.
            /// 
            /// @param [in] weight matrix (TensorTwoD instance)
            ///
            /// @param [in] bias vector (TensorOneD instance)
            ///
            /// @throws geopm::Exception if weights is empty
            ///
            /// @throws geopm::Exception if weights and biases dimensions are incompatible.
            ///         i.e. if weights rows is inequal to bias dimension.
            static std::unique_ptr<DenseLayer> make_unique(const TensorTwoD &weights, const TensorOneD &biases);
            virtual ~DenseLayer() = default;
            /// @brief Perform inference using the instance weights and biases.
            /// 
            /// @param [in] input TensorOneD vector of input signals.
            ///
            /// @throws geopm::Exception if input dimension is incompatible
            ///         with the DenseLayer.
            ///
            /// @return Returns a TensorOneD vector of output values.
            virtual TensorOneD forward(const TensorOneD &input) const = 0;
            /// @brief Get the dimension required for the input TensorOneD
            /// 
            /// @return Returns a size_t equal to the number of columns of weights
            virtual size_t get_input_dim() const = 0;
            /// @brief Get the dimension of the resulting TensorOneD
            /// 
            /// @return Returns a size_t equal to the number of rows of weights 
            virtual size_t get_output_dim() const = 0;

            TensorOneD operator()(const TensorOneD &input) const {
                return forward(input);
            }
    };
}

#endif /* DENSELAYER_HPP_INCLUDE */
