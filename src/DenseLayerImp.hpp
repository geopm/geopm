/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DENSELAYERIMP_HPP_INCLUDE
#define DENSELAYERIMP_HPP_INCLUDE

#include "DenseLayer.hpp"

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

namespace geopm
{
    class DenseLayerImp : public DenseLayer
    {
        public:
            DenseLayerImp(const DenseLayerImp &other);
            DenseLayerImp &operator=(DenseLayerImp other);
            /// @brief Constructor ingesting a TensoTwoD and TensorOneD object
            ///
            /// @param [in] weights The TensorTwoD object containing weights
            ///
            /// @param [in] biases The TensorOneD object containing biases
            ///
            /// @throws geopm::Exception if the weights is empty
            ///
            /// @throws geopm::Exception if the dimensions are incompatible,
            ///         i.e. if weights rows is inequal to bias dimension.
            DenseLayerImp(const TensorTwoD &weights, const TensorOneD &biases);
            /// @brief Inference step
            ///
            /// @param [in] input The TensorOneD object operating upon the DenseLayer
            ///
            /// @throws geopm::Exception if input vector is incompatible with the DenseLayer
            ///
            /// @returns Returns a TensorOneD object of output values
            TensorOneD forward(const TensorOneD &input) const override;
            /// @brief Get the dimension required for the input TensorOneD
            /// 
            /// @return Returns a size_t equal to the number of columns of weights
            size_t get_input_dim() const override;
            /// @brief Get the dimension of the resulting TensorOneD
            /// 
            /// @return Returns a size_t equal to the number of rows of weights 
            size_t get_output_dim() const override;

        private:
            TensorTwoD m_weights;
            TensorOneD m_biases;
    };
}

#endif /* DENSELAYERIMP_HPP_INCLUDE */
