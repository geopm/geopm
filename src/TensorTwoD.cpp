/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "geopm/Exception.hpp"

namespace geopm
{
    TensorTwoD::TensorTwoD()
        : TensorTwoD(0, 0)
    {
    }

    TensorTwoD::TensorTwoD(size_t rows, size_t cols)
        : TensorTwoD(rows, cols, TensorMath::make_shared())
    {
    }

    TensorTwoD::TensorTwoD(size_t rows,
                           size_t cols,
                           std::shared_ptr<TensorMath> math)
        : m_math(math)
    {
        set_dim(rows, cols);
    }

    TensorTwoD::TensorTwoD(const TensorTwoD &other)
        : TensorTwoD(other.m_mat, other.m_math)
    {
    }

    TensorTwoD::TensorTwoD(TensorTwoD &&other)
        : TensorTwoD(std::move(other.m_mat), std::move(other.m_math))
    {
    }

    TensorTwoD::TensorTwoD(const std::vector<TensorOneD> &input)
        : TensorTwoD(input, TensorMath::make_shared())
    {
    }

    TensorTwoD::TensorTwoD(const std::vector<TensorOneD> &input,
                           std::shared_ptr<TensorMath> math)
        : m_math(math)
    {
        set_data(input);
    }

    TensorTwoD::TensorTwoD(const std::vector<std::vector<double>> &input)
        : TensorTwoD(input, TensorMath::make_shared())
    {
    }

    TensorTwoD::TensorTwoD(const std::vector<std::vector<double>> &input,
                           std::shared_ptr<TensorMath> math)
        : m_math(math)
    {
        if (input.size() == 0) {
            throw Exception("TensorTwoD::" + std::string(__func__) +
                            ": Empty array is invalid for neural network weights.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        size_t rows = input.size();
        std::vector<TensorOneD> tensor_vec(rows);
        for (size_t idx = 0; idx < rows; ++idx) {
            tensor_vec[idx] = TensorOneD(input[idx]);
        }

        set_data(tensor_vec);
    }

    size_t TensorTwoD::get_rows() const
    {
        return m_mat.size();
    }

    size_t TensorTwoD::get_cols() const
    {
        if (m_mat.size() == 0) {
            return 0;
        }
        return m_mat[0].get_dim();
    }

    void TensorTwoD::set_dim(size_t rows, size_t cols)
    {
        if ((rows == 0 && cols > 0) || (rows > 0 && cols == 0)) {
            throw Exception("TensorTwoD::" + std::string(__func__) +
                            ": Tried to allocate degenerate matrix.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_mat.resize(rows);
        for (auto &row : m_mat) {
            row.set_dim(cols);
        }
    }

    TensorOneD TensorTwoD::operator*(const TensorOneD &other) const
    {
        return m_math->multiply(*this, other);
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
        m_math = other.m_math;

        return *this;
    }

    bool TensorTwoD::operator==(const TensorTwoD &other) const
    {
        return m_mat == other.m_mat;
    }

    const std::vector<TensorOneD> &TensorTwoD::get_data() const
    {
        return m_mat;
    }

    void TensorTwoD::set_data(const std::vector<TensorOneD> &data)
    {
        m_mat = data;
        size_t rows = data.size();
        if (rows > 1) {
            size_t cols = data[0].get_dim();
            for (size_t idx = 1; idx < rows; ++idx) {
                if (data[idx].get_dim() != cols) {
                    throw Exception("TensorTwoD::" + std::string(__func__) +
                                    ": Attempt to load non-rectangular matrix.",
                                    GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }
    }
}
