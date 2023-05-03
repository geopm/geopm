/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DenseLayer.hpp"
#include "DenseLayerImp.hpp"

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    std::unique_ptr<DenseLayer> DenseLayer::make_unique(const TensorTwoD &weights, const TensorOneD &biases)
    {
        return geopm::make_unique<DenseLayerImp>(weights, biases);
    }

    DenseLayerImp::DenseLayerImp(const DenseLayerImp &other)
        : DenseLayerImp(other.m_weights, other.m_biases)
    {
    }

    DenseLayerImp::DenseLayerImp(const TensorTwoD &weights, const TensorOneD &biases) 
	    : m_weights(weights)
	      , m_biases(biases)
    {
        if (weights.get_rows() == 0 && weights.get_cols() == 0) {
            throw Exception("Empty array is invalid for neural network weights.\n",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        if (weights.get_rows() != biases.get_dim()) {
            throw Exception("Incompatible dimensions for weights and biases.\n",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    TensorOneD DenseLayerImp::forward(const TensorOneD &input) const
    {
        if (input.get_dim() != m_weights.get_cols()) {
            throw Exception("Input vector dimension is incompatible with network.\n",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return m_biases + m_weights * input;
    }

    size_t DenseLayerImp::get_input_dim() const
    {
        return m_weights.get_cols();
    }

    size_t DenseLayerImp::get_output_dim() const
    {
        return m_weights.get_rows();
    }
}
