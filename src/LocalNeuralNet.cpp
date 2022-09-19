/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include "geopm/Exception.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "LocalNeuralNet.hpp"

LocalNeuralNet::LocalNeuralNet()
{
}

LocalNeuralNet::LocalNeuralNet(const LocalNeuralNet &other)
{
    m_layers.resize(other.m_layers.size());
    for (int idx = 0; idx < m_layers.size(); idx++) {
        m_layers[idx] = other.m_layers[idx];
    }
}

LocalNeuralNet::LocalNeuralNet(json11::Json input)
{
    m_layers.resize(input.array_items().size());
    for (int idx = 0; idx < m_layers.size(); idx++) {
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

LocalNeuralNet& LocalNeuralNet::operator=(const LocalNeuralNet &other)
{
    m_layers.resize(other.m_layers.size());
    for (int idx = 0; idx < m_layers.size(); idx++) {
        m_layers[idx] = other.m_layers[idx];
    }
    return *this;
}

TensorOneD LocalNeuralNet::model(TensorOneD inp)
{
    TensorOneD tmp = inp;
    for (int idx = 0; idx < m_layers.size(); idx++) {
        // Tensor operations
        tmp = m_layers[idx].first * tmp + m_layers[idx].second;

        if (idx != m_layers.size() - 1) {
            tmp = tmp.sigmoid();
        }
    }
    return tmp;
}
