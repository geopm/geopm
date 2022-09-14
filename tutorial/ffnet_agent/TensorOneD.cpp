/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>

#include "TensorOneD.hpp"

TensorOneD::TensorOneD()
{
}

TensorOneD::TensorOneD(int dim)
{
    set_dim(dim);
}

TensorOneD::TensorOneD(const TensorOneD &other)
{
    set_dim(other.m_dim);
    for (int ii = 0; ii < m_dim; ii++) {
        m_vec[ii] = other[ii];
    }
}

TensorOneD::TensorOneD(json11::Json input)
{
    // TODO verify input
    set_dim(input.array_items().size());
    for (int ii = 0; ii < m_dim; ii++) {
        m_vec[ii] = input[ii].number_value();
    }
}

int
TensorOneD::get_dim()
{
    return m_dim;
}

void
TensorOneD::set_dim(int dim)
{
    m_vec.resize(dim);
    m_dim = dim;
}

TensorOneD
TensorOneD::operator+(const TensorOneD& other)
{
    TensorOneD rval(m_dim);
    // assert(this->n == other.n);
    for (int ii = 0; ii < m_dim; ii++) {
        rval[ii] = m_vec[ii] + other.m_vec[ii];
    }

    return rval;
}

TensorOneD
TensorOneD::operator-(const TensorOneD& other)
{
    TensorOneD rval(m_dim);
    for (int ii = 0; ii < m_dim; ii++) {
        rval[ii] = m_vec[ii] - other.m_vec[ii];
    }

    return rval;
}


float
TensorOneD::operator*(const TensorOneD& other)
{
    float rval = 0;
    // assert(this->n == other.n);
    for (int ii = 0; ii < m_dim; ii++) {
        rval += m_vec[ii] * other.m_vec[ii];
    }

    return rval;
}

TensorOneD&
TensorOneD::operator=(const TensorOneD &other)
{
    set_dim(other.m_dim);
    for (int ii = 0; ii < m_dim; ii++) {
        m_vec[ii] = other[ii];
    }
    return *this;
}

float&
TensorOneD::operator[] (int ii)
{
    return m_vec[ii];
}

float
TensorOneD::operator[] (int ii) const
{
    return m_vec[ii];
}

TensorOneD
TensorOneD::sigmoid()
{
    TensorOneD rval(m_dim);
    for(int ii = 0; ii < m_dim; ii++) {
        rval[ii] = 1/(1 + expf(-(m_vec[ii])));
    }
    return rval;
}
