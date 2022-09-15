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
    for (int ii = 0; ii < m_nlayers; ii++) {
        m_weights[ii] = other.m_weights[ii];
        m_biases[ii] = other.m_biases[ii];
    }
}

LocalNeuralNet::LocalNeuralNet(json11::Json input)
{
    // TODO validate by throwing exceptions
    m_nlayers = input.array_items().size();
    m_weights.resize(m_nlayers);
    m_biases.resize(m_nlayers);
    for (int ii = 0; ii < m_nlayers; ii++) {
        m_weights[ii] = TensorTwoD(input[ii][0]);
        m_biases[ii] = TensorOneD(input[ii][1]);
    }
}

LocalNeuralNet&
LocalNeuralNet::operator=(const LocalNeuralNet &other)
{
    m_nlayers = other.m_nlayers;
    m_weights.resize(m_nlayers);
    m_biases.resize(m_nlayers);
    for (int ii = 0; ii < m_nlayers; ii++) {
        m_weights[ii] = other.m_weights[ii];
        m_biases[ii] = other.m_biases[ii];
    }
}

TensorOneD
LocalNeuralNet::model(TensorOneD inp)
{
    TensorOneD tmp;

    tmp = inp;
    for (int ii = 0; ii < m_nlayers; ii++) {
        // Tensor operations
        tmp = m_weights[ii] * tmp + m_biases[ii];

        if (ii != m_nlayers - 1) {
            tmp = tmp.sigmoid();
        }
    }

    return tmp;
}
