/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"
#include "TensorTwoD.hpp"

#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>

#include "geopm/Exception.hpp"
#include "TensorOneD.hpp"

namespace geopm
{

    TensorTwoD::TensorTwoD(std::size_t rows, std::size_t cols)
    {
        set_dim(rows, cols);
    }

    TensorTwoD::TensorTwoD(const TensorTwoD &other)
        : m_mat(other.m_mat)
    {
    }

    TensorTwoD::TensorTwoD(json11::Json input)
    {
        if (!input.is_array()) {
            throw geopm::Exception("Neural network weights is non-array-type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (input.array_items().size() == 0) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (!input[0].is_array()) {
            throw geopm::Exception("Neural network weights is non-array-type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        set_dim(input.array_items().size(), input[0].array_items().size());
        for (std::size_t idx = 0; idx < m_mat.size(); ++idx) {
            if(!input[idx].is_array()) {
                throw geopm::Exception("Neural network weights is non-array-type.\n",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (input[idx].array_items().size() != m_mat[idx].get_dim()) {
                throw geopm::Exception("Attempt to load non-rectangular matrix.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_mat[idx] = TensorOneD(input[idx]);
        }
    }

    std::size_t TensorTwoD::get_rows() const
    {
        return m_mat.size();
    }

    std::size_t TensorTwoD::get_cols() const
    {
        if (m_mat.size() == 0) {
            return 0;
        }
        return m_mat[0].get_dim();
    }

    void TensorTwoD::set_dim(std::size_t rows, std::size_t cols)
    {
        if ((rows == 0 && cols > 0) || (rows > 0 && cols == 0)) {
            throw geopm::Exception("Tried to allocate degenerate matrix.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_mat.resize(rows);
        for (auto &row : m_mat) {
            row.set_dim(cols);
        }
    }


    TensorOneD TensorTwoD::operator*(const TensorOneD& other)
    {
        if (get_cols() != other.get_dim()) {
            throw geopm::Exception("Attempted to multiply matrix and vector with incompatible dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorOneD rval(get_rows());

        for (std::size_t idx = 0; idx < get_rows(); ++idx) {
            rval[idx] = m_mat[idx] * other;
        }
        return rval;
    }

    TensorOneD& TensorTwoD::operator[](size_t idx)
    {
        return m_mat.at(idx);
    }

    TensorOneD TensorTwoD::operator[](size_t idx) const
    {
        return m_mat.at(idx);
    }

    TensorTwoD& TensorTwoD::operator=(const TensorTwoD &other)
    {
        m_mat = other.m_mat;

        return *this;
    }
}
