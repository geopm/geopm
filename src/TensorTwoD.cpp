/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <math.h>

#include "TensorTwoD.hpp"
#include "TensorOneD.hpp"

TensorTwoD::TensorTwoD()
{
    m_rows = m_cols = 0;
}

TensorTwoD::TensorTwoD(int rows, int cols)
{
    set_dim(rows, cols);
}

TensorTwoD::TensorTwoD(const TensorTwoD &other)
{
    set_dim(other.m_rows, other.m_cols);
    for (int idx_dim = 0; idx_dim < m_rows; idx_dim++) {
        for (int jj = 0; jj < m_cols; jj++) {
            m_mat[idx_dim][jj] = other.m_mat[idx_dim][jj];
        }
    }
}

TensorTwoD::TensorTwoD(json11::Json input)
{
    // TODO verify input
    set_dim(input.array_items().size(), input[0].array_items().size());
    for (int idx_dim = 0; idx_dim < m_rows; idx_dim++) {
        for (int jj = 0; jj < m_cols; jj++) {
            m_mat[idx_dim][jj] = input[idx_dim][jj].number_value();
        }
    }
}

void
TensorTwoD::set_dim(int rows, int cols)
{
    m_rows = rows;
    m_cols = cols;
    m_mat.resize(m_rows);
    for (int idx_dim = 0; idx_dim < m_rows; idx_dim++) {
        m_mat[idx_dim].set_dim(cols);
    }
}

int
TensorTwoD::get_rows()
{
    return m_rows;
}

int
TensorTwoD::get_cols()
{
    return m_cols;
}

TensorOneD
TensorTwoD::operator*(const TensorOneD& other)
{
    TensorOneD rval(m_rows);
    for (int idx_dim = 0; idx_dim < m_rows; idx_dim++) {
        rval[idx_dim] = m_mat[idx_dim] * other;
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
    set_dim(other.m_rows, other.m_cols);
    for (int idx_dim = 0; idx_dim < m_rows; idx_dim++) {
        m_mat[idx_dim] = other.m_mat[idx_dim];
    }
    return *this;
}
