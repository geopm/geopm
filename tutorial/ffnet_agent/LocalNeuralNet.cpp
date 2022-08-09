/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>
#include <math.h>

#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"
#include "LocalNeuralNet.hpp"

LocalNeuralNet::LocalNeuralNet() {
}

LocalNeuralNet::LocalNeuralNet(const LocalNeuralNet &that) {
    this->allocated = that.allocated;
    if (that.allocated) {
        this->nlayers = that.nlayers;
        this->weights = new TensorTwoD[this->nlayers];
        this->biases = new TensorOneD[this->nlayers];
        for (int i=0; i<this->nlayers; i++) {
            this->weights[i] = that.weights[i];
            this->biases[i] = that.biases[i];
        }
    }
}

TensorOneD
LocalNeuralNet::model(TensorOneD inp) {
    TensorOneD tmp;

    tmp = inp;
    for (int i=0; i<this->nlayers; i++) {
        tmp = this->weights[i] * tmp;
        tmp = tmp + this->biases[i];


        if (i != nlayers - 1) {
            tmp = tmp.sigmoid();
        }
    }

    return tmp;
}

std::ostream & operator << (std::ostream &out, const LocalNeuralNet &n) {
    out << 2*n.nlayers << std::endl;
    for (int i=0; i<n.nlayers; i++) {
        out << n.weights[i] << std::endl;
        out << n.biases[i] << std::endl;
    }
    return out;
}

std::istream & operator >> (std::istream &in, LocalNeuralNet &n) {
    in >> n.nlayers;
    n.nlayers /= 2;
    n.weights = new TensorTwoD[n.nlayers];
    n.biases = new TensorOneD[n.nlayers];
    for (int i=0; i<n.nlayers; i++) {
        in >> n.weights[i];
        in >> n.biases[i];
    }
    n.allocated = true;
    return in;
}

LocalNeuralNet::~LocalNeuralNet() {
    if (allocated && false) {
        delete[] this->weights;
        delete[] this->biases;
    }
}

LocalNeuralNet read_nnet(std::string nn_path) {
    std::ifstream inp(nn_path);
    LocalNeuralNet lnn;
    inp >> lnn;
    return lnn;
}
