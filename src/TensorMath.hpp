/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORMATH_HPP_INCLUDE
#define TENSORMATH_HPP_INCLUDE

#include <vector>

namespace geopm
{
    class TensorOneD;

    /// @brief Class to perform operations on 1D and 2D Tensors,
    ///        aka vectors and matrices, suitable for use in
    ///        feed-forward neural networks.
    class TensorMath
    {
        public:
            /// @brief Add two 1D tensors, element-wise
            ///
            /// The tensors need to be the same length. 
            ///
            /// @throws geopm::Exception if the lengths do not match.
            ///
            /// @param [in] other The summand
            ///
            /// @return Returns a 1D tensor, the sum of two 1D tensors
            virtual TensorOneD add(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const = 0;
            /// @brief Subtract two 1D tensors, element-wise
            ///
            /// @throws geopm::Exception if the lengths do not match.
            ///
            /// @param [in] other The subtrahend
            ///
            /// @return A 1D tensor, the difference of two 1D tensors.
            virtual TensorOneD subtract(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const = 0;
            /// @brief Multiply two 1D tensors, element-wise, and sum the result.
            ///
            /// @throws geopm::Exception if the lengths do not match.
            ///
            /// @param [in] other The multiplicand
            ///
            /// @return Returns a 1D tensor, the product of two 1D tensors
            virtual float inner_product(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const = 0;
            /// @brief Compute logistic sigmoid function of 1D Tensor

            virtual TensorOneD sigmoid(const TensorOneD tensor) const = 0;
    };

    class TensorMathImp : public TensorMath
    {
        public:
            TensorMathImp() = default;
            virtual ~TensorMathImp() = default;
            TensorOneD add(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const override;
            TensorOneD subtract(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const override;
            float inner_product(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const override;
            TensorOneD sigmoid(const TensorOneD tensor) const override;
    };
}
#endif /* TENSORMATH_HPP_INCLUDE */
