/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LOCALNEURALNET_HPP_INCLUDE
#define LOCALNEURALNET_HPP_INCLUDE

#include "geopm/json11.hpp"

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

class LocalNeuralNet
{
    public:
        LocalNeuralNet();
        LocalNeuralNet(const LocalNeuralNet&);
        LocalNeuralNet(const json11::Json input);
        LocalNeuralNet& operator=(const LocalNeuralNet&);
        TensorOneD model(TensorOneD inp);

    private:
        std::vector<std::pair<TensorTwoD, TensorOneD> > m_layers;
};

#endif
