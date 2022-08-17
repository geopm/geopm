/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>
#include <math.h>

#include "TensorOneD.hpp"

TensorOneD::TensorOneD() {
    this->allocated = false;
}

TensorOneD::TensorOneD(int n) {
    this->allocated = false;
    this->set_dim(n);
}

TensorOneD::TensorOneD(const TensorOneD &that) {
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.n);
        for (int i=0; i<n; i++) {
            this->vec[i] = that[i];
        }
    }
}

TensorOneD::~TensorOneD() {
    if (this->allocated) {
        delete[] this->vec;
        this->allocated = false;
    }
}

int
TensorOneD::get_dim() {
    return this->n;
}

void
TensorOneD::set_dim(int n) {
    if (this->allocated) {
        delete[] this->vec;
    }
    this->vec = new float[n];
    this->n = n;
    this->allocated = true;
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
    if (this->allocated) {
        delete[] this->vec;
    }
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.n);
        for (int i=0; i<n; i++) {
            this->vec[i] = that[i];
        }
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

std::ostream & operator << (std::ostream &out, const TensorOneD &v) {
    out << "1 " << v.n << " ";

    for (int i=0; i<v.n; i++) {
        out << v[i] << " ";
    }
    return out;
}

std::istream & operator >> (std::istream &in, TensorOneD &v) {
    int one, n;
    in >> one >> n;
    v.set_dim(n);
    for (int i=0; i<n; i++) {
        in >> v[i];
    }
    return in;
}
