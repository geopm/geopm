/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LOCALNEURALNET_HPP_INCLUDE 
#define LOCALNEURALNET_HPP_INCLUDE 

#include <iostream>

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

class LocalNeuralNet
{
    public:
        LocalNeuralNet();
        LocalNeuralNet(const LocalNeuralNet&);
        TensorOneD model(TensorOneD inp);
        friend std::ostream & operator << (std::ostream &out, const LocalNeuralNet &n);
        friend std::istream & operator >> (std::istream &in, LocalNeuralNet &n);
        ~LocalNeuralNet();

    private:
        TensorTwoD *weights;
        TensorOneD *biases;
        int nlayers;
        bool allocated;
};

LocalNeuralNet read_nnet(std::string nn_path);

#endif
