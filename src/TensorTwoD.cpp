/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include "geopm/Exception.hpp"

#include <iostream>

namespace geopm
{
    TensorTwoD::TensorTwoD()
        : TensorTwoD(0, 0)
    {
    }

    TensorTwoD::TensorTwoD(size_t rows, size_t cols)
        : TensorTwoD(rows, cols, std::make_shared<TensorMathImp>())
    {
    }

    TensorTwoD::TensorTwoD(size_t rows,
                           size_t cols,
                           std::shared_ptr<TensorMath> math)
    {
        set_dim(rows, cols);
        m_math = math;
    }

    TensorTwoD::TensorTwoD(const TensorTwoD &other)
        : m_mat(other.m_mat)
    {
    }

    TensorTwoD::TensorTwoD(TensorTwoD &&other)
        : TensorTwoD(std::move(other.m_mat))
    {
    }

    TensorTwoD::TensorTwoD(std::vector<TensorOneD> input)
        : TensorTwoD(input, std::make_shared<TensorMathImp>())
    {
    }

    TensorTwoD::TensorTwoD(std::vector<TensorOneD> input,
                           std::shared_ptr<TensorMath> math)
    {
        set_data(input);
        m_math = math;
    }

    TensorTwoD::TensorTwoD(std::vector<std::vector<float> > input)
    {
        std::cerr << "init 61" << std::endl;
        std::cout << "init 62" << std::endl;

        if (input.size() == 0) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::cout << "init 69" << std::endl;

        size_t rows = input.size();
        std::cout << "rows = " << rows << std::endl;
        std::vector<TensorOneD> tensor_vec(rows);
        for (size_t idx = 0; idx < rows; ++idx) {
            tensor_vec[idx] = TensorOneD(input[idx]);
            std::cout << "cols[" << idx << "] = " << tensor_vec[idx].get_dim() << std::endl;
            std::cout << "from " << input[idx].size() << std::endl;
        }

        set_data(tensor_vec);
        m_math = std::make_shared<TensorMathImp>();
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

    void TensorTwoD::set_data(const std::vector<TensorOneD> data)
    {
        m_mat = data;
        size_t rows = data.size();
        if (rows > 1) {
            std::cout << "hi" << std::endl;
            size_t cols = data[0].get_dim();
            for (size_t idx = 1; idx < rows; ++idx) {
                std::cout << idx << data[idx].get_dim() << "=?" << cols << std::endl;
                if (data[idx].get_dim() != cols) {
                    throw geopm::Exception("Attempt to load non-rectangular matrix.",
                                           GEOPM_ERROR_INVALID, __FILE__, __LINE__);
                }
            }
        }
    }
}
