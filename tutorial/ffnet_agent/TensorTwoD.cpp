/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fstream>
#include <math.h>

#include "TensorTwoD.hpp"
#include "TensorOneD.hpp"

TensorTwoD::TensorTwoD() {
    this->allocated = false;
}

TensorTwoD::TensorTwoD(int r, int c) {
    this->allocated = false;
    this->set_dim(r, c);
}

TensorTwoD::TensorTwoD(const TensorTwoD &that) {
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.r, that.c);
        for (int i=0; i<this->r; i++) {
            for (int j=0; j<this->c; j++) {
                this->mat[i][j] = that.mat[i][j];
            }
        }
    }
}

TensorTwoD::~TensorTwoD() {
    if (this->allocated) {
        delete[] this->mat;
        this->allocated = false;
    }
}

void
TensorTwoD::set_dim(int r, int c) {
    if (this->allocated) {
        delete[] this->mat;
    }
    this->r = r;
    this->c = c;
    this->mat = new TensorOneD[r];
    for (int i=0; i<r; i++) {
        this->mat[i].set_dim(c);
    }
    this->allocated = true;
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
    if (this->allocated) {
        delete[] this->mat;
    }
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.r, that.c);
        for (int i=0; i<r; i++) {
            for (int j=0; j<c; j++) {
                this->mat[i][j] = that.mat[i][j];
            }
        }
    }
    return *this;
}


std::ostream & operator << (std::ostream &out, const TensorTwoD &m) {
    out << "2 " << m.r << " " << m.c << " ";
    for (int i=0; i<m.r; i++) {
        for (int j=0; j<m.c; j++) {
            out << m[i][j] << " ";
        }
    }
    return out;
}

std::istream & operator >> (std::istream &in, TensorTwoD &m) {
    int two, r, c;
    in >> two >> r >> c;
    m.set_dim(r, c);
    for (int i=0; i<r; i++) {
        for (int j=0; j<c; j++) {
            in >> m[i][j];
        }
    }
    return in;
}

