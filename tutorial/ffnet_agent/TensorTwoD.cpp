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
    for (int idx_row = 0; idx_row < m_rows; idx_row++) {
        for (int idx_col = 0; idx_col < m_cols; idx_col++) {
            m_mat[idx_row][idx_col] = other.m_mat[idx_row][idx_col];
        }
    }
}

TensorTwoD::TensorTwoD(json11::Json input)
{
    // TODO verify input
    set_dim(input.array_items().size(), input[0].array_items().size());
    for (int idx_row = 0; idx_row < m_rows; idx_row++) {
        for (int idx_col = 0; idx_col < m_cols; idx_col++) {
            m_mat[idx_row][idx_col] = input[idx_row][idx_col].number_value();
        }
    }
}

void
TensorTwoD::set_dim(int rows, int cols)
{
    m_rows = rows;
    m_cols = cols;
    m_mat.resize(m_rows);
    for (int idx_row = 0; idx_row < m_rows; idx_row++) {
        m_mat[idx_row].set_dim(cols);
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
    for (int idx_row = 0; idx_row < m_rows; idx_row++) {
        rval[idx_row] = m_mat[idx_row] * other;
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
    for (int idx_row = 0; idx_row < m_rows; idx_row++) {
        m_mat[idx_row] = other.m_mat[idx_row];
    }
    return *this;
}
