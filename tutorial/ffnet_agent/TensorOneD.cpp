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
    for (int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        m_vec[idx_dim] = other[idx_dim];
    }
}

TensorOneD::TensorOneD(json11::Json input)
{
    // TODO verify input
    set_dim(input.array_items().size());
    for (int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        m_vec[idx_dim] = input[idx_dim].number_value();
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
    for (int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        rval[idx_dim] = m_vec[idx_dim] + other.m_vec[idx_dim];
    }

    return rval;
}

TensorOneD
TensorOneD::operator-(const TensorOneD& other)
{
    TensorOneD rval(m_dim);
    for (int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        rval[idx_dim] = m_vec[idx_dim] - other.m_vec[idx_dim];
    }

    return rval;
}


float
TensorOneD::operator*(const TensorOneD& other)
{
    float rval = 0;
    // assert(this->n == other.n);
    for (int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        rval += m_vec[idx_dim] * other.m_vec[idx_dim];
    }

    return rval;
}

TensorOneD&
TensorOneD::operator=(const TensorOneD &other)
{
    set_dim(other.m_dim);
    for (int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        m_vec[idx_dim] = other[idx_dim];
    }
    return *this;
}

float&
TensorOneD::operator[] (int idx)
{
    return m_vec[idx];
}

float
TensorOneD::operator[] (int idx) const
{
    return m_vec[idx];
}

TensorOneD
TensorOneD::sigmoid()
{
    TensorOneD rval(m_dim);
    for(int idx_dim = 0; idx_dim < m_dim; idx_dim++) {
        rval[idx_dim] = 1/(1 + expf(-(m_vec[idx_dim])));
    }
    return rval;
}
