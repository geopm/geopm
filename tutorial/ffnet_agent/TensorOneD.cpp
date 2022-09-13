/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>

#include "TensorOneD.hpp"

TensorOneD::TensorOneD() {
}

TensorOneD::TensorOneD(int n) {
    this->set_dim(n);
}

TensorOneD::TensorOneD(const TensorOneD &that) {
    this->set_dim(that.n);
    for (int i=0; i<n; i++) {
        this->vec[i] = that[i];
    }
}

TensorOneD::TensorOneD(json11::Json input) {
    // TODO verify input
    this->set_dim(input.array_items().size());
    for (int i=0; i<this->n; i++) {
        this->vec[i] = input[i].number_value();
    }
}

int
TensorOneD::get_dim() {
    return this->n;
}

void
TensorOneD::set_dim(int n) {
    this->vec.resize(n);
    this->n = n;
}

TensorOneD
TensorOneD::operator+(const TensorOneD& that) {
    TensorOneD rval(this->n);
    // assert(this->n == that.n);
    for (int i=0; i<n; i++) {
        rval[i] = this->vec[i] + that.vec[i];
    }

    return rval;
}

TensorOneD
TensorOneD::operator-(const TensorOneD& that) {
    TensorOneD rval(this->n);
    for (int i=0; i<this->n; i++) {
        rval[i] = this->vec[i] - that.vec[i];
    }

    return rval;
}


float
TensorOneD::operator*(const TensorOneD& that) {
    float rval = 0;
    // assert(this->n == that.n);
    for (int i=0; i<this->n; i++) {
        rval += this->vec[i] * that.vec[i];
    }

    return rval;
}

TensorOneD&
TensorOneD::operator=(const TensorOneD &that) {
    this->set_dim(that.n);
    for (int i=0; i<n; i++) {
        this->vec[i] = that[i];
    }
    return *this;
}

float&
TensorOneD::operator[] (int i) {
    return this->vec[i];
}

float
TensorOneD::operator[] (int i) const {
    return this->vec[i];
}

TensorOneD
TensorOneD::sigmoid() {
    TensorOneD rval;
    rval.set_dim(n);
    for(int i=0; i<n; i++) {
        rval[i] = 1/(1 + expf(-(this->vec[i])));
    }
    return rval;
}
