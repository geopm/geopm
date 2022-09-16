/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include "geopm/Exception.hpp"

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
    set_dim(other.get_dim());
    for (int idx = 0; idx < m_vec.size(); idx++) {
        m_vec[idx] = other[idx];
    }
}

TensorOneD::TensorOneD(json11::Json input)
{
    set_dim(input.array_items().size());
    for (int idx = 0; idx < m_vec.size(); idx++) {
        m_vec[idx] = input[idx].number_value();
    }
}

int
TensorOneD::get_dim() const
{
    return m_vec.size();
}

void
TensorOneD::set_dim(int dim)
{
    m_vec.resize(dim);
}

TensorOneD
TensorOneD::operator+(const TensorOneD& other)
{
    if (get_dim() != other.get_dim()) {
        throw geopm::Exception("Adding vectors of mismatched dimensions.",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    TensorOneD rval(m_vec.size());
    for (int idx = 0; idx < m_vec.size(); idx++) {
        rval[idx] = m_vec[idx] + other.m_vec[idx];
    }

    return rval;
}

TensorOneD
TensorOneD::operator-(const TensorOneD& other)
{
    if (get_dim() != other.get_dim()) {
        throw geopm::Exception("Subtracting vectors of mismatched dimensions.",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    TensorOneD rval(m_vec.size());
    for (int idx = 0; idx < m_vec.size(); idx++) {
        rval[idx] = m_vec[idx] - other.m_vec[idx];
    }

    return rval;
}


float
TensorOneD::operator*(const TensorOneD& other)
{
    if (get_dim() != other.get_dim()) {
        throw geopm::Exception("Inner product of vectors of mismatched dimensions.",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    float rval = 0;
    // assert(this->n == other.n);
    for (int idx = 0; idx < m_vec.size(); idx++) {
        rval += m_vec[idx] * other.m_vec[idx];
    }

    return rval;
}

TensorOneD&
TensorOneD::operator=(const TensorOneD &other)
{
    set_dim(other.get_dim());
    for (int idx = 0; idx < m_vec.size(); idx++) {
        m_vec[idx] = other[idx];
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
TensorOneD::sigmoid() const
{
    TensorOneD rval(m_vec.size());
    for(int idx = 0; idx < m_vec.size(); idx++) {
        rval[idx] = 1/(1 + expf(-(m_vec[idx])));
    }
    return rval;
}
