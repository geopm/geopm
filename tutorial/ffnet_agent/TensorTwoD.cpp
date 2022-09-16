/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>
#include "geopm/Exception.hpp"
#include "TensorOneD.hpp"

#include "TensorTwoD.hpp"

TensorTwoD::TensorTwoD()
{
}

TensorTwoD::TensorTwoD(int rows, int cols)
{
    set_dim(rows, cols);
}

TensorTwoD::TensorTwoD(const TensorTwoD &other)
{
    set_dim(other.get_rows(), other.get_cols());
    for (int idx = 0; idx < m_mat.size(); idx++) {
        m_mat[idx] = other.m_mat[idx];
    }
}

TensorTwoD::TensorTwoD(json11::Json input)
{
    set_dim(input.array_items().size(), input[0].array_items().size());
    for (int idx = 0; idx < m_mat.size(); idx++) {
        if (input[idx].array_items().size() != m_mat[idx].get_dim()) {
            throw geopm::Exception("Attempt to load non-rectangular matrix.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_mat[idx] = TensorOneD(input[idx]);
    }
}

void
TensorTwoD::set_dim(int rows, int cols)
{
    if (rows == 0 && cols > 0) {
        throw geopm::Exception("Tried to allocate degenerate matrix.",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    m_mat.resize(rows);
    for (int idx = 0; idx < rows; idx++) {
        m_mat[idx].set_dim(cols);
    }
}

int
TensorTwoD::get_rows() const
{
    return m_mat.size();
}

int
TensorTwoD::get_cols() const
{
    if (m_mat.size() == 0) {
        return 0;
    }
    return m_mat[0].get_dim();
}

TensorOneD
TensorTwoD::operator*(const TensorOneD& other)
{
    if (get_cols() != other.get_dim()) {
        throw geopm::Exception("Attempted to multiply matrix and vector with incompatible dimensions.",
                               GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    TensorOneD rval(get_rows());
    for (int idx = 0; idx < get_rows(); idx++) {
        rval[idx] = m_mat[idx] * other;
    }
    return rval;
}

TensorOneD&
TensorTwoD::operator[](int idx)
{
    return m_mat[idx];
}

TensorOneD
TensorTwoD::operator[](int idx) const
{
    return m_mat[idx];
}

TensorTwoD&
TensorTwoD::operator=(const TensorTwoD &other)
{
    set_dim(other.get_rows(), other.get_cols());
    for (int idx = 0; idx < get_rows(); idx++) {
        m_mat[idx] = other.m_mat[idx];
    }
    return *this;
}
