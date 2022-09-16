/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"
#include "LocalNeuralNet.hpp"

LocalNeuralNet::LocalNeuralNet()
{
    m_nlayers = 0;
}

LocalNeuralNet::LocalNeuralNet(const LocalNeuralNet &other)
{
    m_nlayers = other.m_nlayers;
    m_weights.resize(m_nlayers);
    m_biases.resize(m_nlayers);
    for (int idx_layer = 0; idx_layer < m_nlayers; idx_layer++) {
        m_weights[idx_layer] = other.m_weights[idx_layer];
        m_biases[idx_layer] = other.m_biases[idx_layer];
    }
}

LocalNeuralNet::LocalNeuralNet(json11::Json input)
{
    // TODO validate by throwing exceptions
    m_nlayers = input.array_items().size();
    m_weights.resize(m_nlayers);
    m_biases.resize(m_nlayers);
    for (int idx_layer = 0; idx_layer < m_nlayers; idx_layer++) {
        m_weights[idx_layer] = TensorTwoD(input[idx_layer][0]);
        m_biases[idx_layer] = TensorOneD(input[idx_layer][1]);
    }
}

LocalNeuralNet& LocalNeuralNet::operator=(const LocalNeuralNet &other)
{
    m_nlayers = other.m_nlayers;
    m_weights.resize(m_nlayers);
    m_biases.resize(m_nlayers);
    for (int idx_layer = 0; idx_layer < m_nlayers; idx_layer++) {
        m_weights[idx_layer] = other.m_weights[idx_layer];
        m_biases[idx_layer] = other.m_biases[idx_layer];
    }
}

TensorOneD LocalNeuralNet::model(TensorOneD inp)
{
    TensorOneD tmp;

    tmp = inp;
    for (int idx_layer = 0; idx_layer < m_nlayers; idx_layer++) {
        // Tensor operations
        tmp = m_weights[idx_layer] * tmp + m_biases[idx_layer];

        if (idx_layer != m_nlayers - 1) {
            tmp = tmp.sigmoid();
        }
    }

    return tmp;
}
