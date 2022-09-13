/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>

#include "TensorTwoD.hpp"
#include "TensorOneD.hpp"

TensorTwoD::TensorTwoD() {
    this->r = this->c = 0;
}

TensorTwoD::TensorTwoD(int r, int c) {
    this->set_dim(r, c);
}

TensorTwoD::TensorTwoD(const TensorTwoD &that) {
    this->set_dim(that.r, that.c);
    for (int i=0; i<this->r; i++) {
        for (int j=0; j<this->c; j++) {
            this->mat[i][j] = that.mat[i][j];
        }
    }
}

TensorTwoD::TensorTwoD(json11::Json input) {
    // TODO verify input
    this->set_dim(input.array_items().size(), input[0].array_items().size());
    for (int i=0; i<r; i++) {
        for (int j=0; j<c; j++) {
            this->mat[i][j] = input[i][j].number_value();
        }
    }
}

void
TensorTwoD::set_dim(int r, int c) {
    this->r = r;
    this->c = c;
    this->mat.resize(r);
    for (int i=0; i<r; i++) {
        this->mat[i].set_dim(c);
    }
}

int
TensorTwoD::get_rows() {
    return this->r;
}

int
TensorTwoD::get_cols() {
    return this->c;
}

TensorOneD
TensorTwoD::operator*(const TensorOneD& that) {
    TensorOneD rval(this->r);
    for (int i=0; i<this->r; i++) {
        rval[i] = this->mat[i] * that;
    }
    return rval;
}

TensorOneD&
TensorTwoD::operator[](int i) {
    return this->mat[i];
}

TensorOneD
TensorTwoD::operator[](int i) const {
    return this->mat[i];
}

TensorTwoD&
TensorTwoD::operator=(const TensorTwoD &that) {
    this->set_dim(that.r, that.c);
    for (int i=0; i<r; i++) {
        this->mat[i] = that.mat[i];
    }
    return *this;
}

