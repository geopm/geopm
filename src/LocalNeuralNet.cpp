/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include <math.h>
#include "geopm/Exception.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "LocalNeuralNet.hpp"

namespace geopm
{
    LocalNeuralNet::LocalNeuralNet(const LocalNeuralNet &other)
        : m_layers(other.m_layers)
    {
    }

    LocalNeuralNet::LocalNeuralNet(json11::Json input)
    {
        if (!input.is_array()) {
            throw geopm::Exception("Neural network weights is non-array type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (input.array_items().size() == 0) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_layers.resize(input.array_items().size());
        for (int idx = 0; idx < (int)m_layers.size(); idx++) {
            if (!input[idx].is_array() || input[idx].array_items().size() != 2) {
                throw geopm::Exception("Neural network weight entries should be doubles.\n",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }

            m_layers[idx] = std::make_pair(TensorTwoD(input[idx][0]), TensorOneD(input[idx][1]));
            if (m_layers[idx].first.get_rows() != m_layers[idx].second.get_dim()) {
                throw geopm::Exception("Incompatible dimensions for weights and biases.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (idx > 0 && m_layers[idx].first.get_cols() != m_layers[idx-1].first.get_rows()) {
                throw geopm::Exception("Incompatible dimensions for consecutive layers.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    LocalNeuralNet&
    LocalNeuralNet::operator=(const LocalNeuralNet &other)
    {
        m_layers = other.m_layers;

        return *this;
    }

    TensorOneD
    LocalNeuralNet::model(TensorOneD inp)
    {
        if (inp.get_dim() != m_layers[0].first.get_cols()) {
            throw geopm::Exception("Input vector dimension is incompatible with network.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorOneD tmp = inp;
        for (int idx = 0; idx < (int)m_layers.size(); idx++) {
            // Tensor operations
            tmp = m_layers[idx].first * tmp + m_layers[idx].second;

            // Apply a sigmoid on all but the last layer
            if (idx != (int)m_layers.size() - 1) {
                tmp = tmp.sigmoid();
            }
        }

        return tmp;
    }
}
