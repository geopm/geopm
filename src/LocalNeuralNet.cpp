/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"
#include "LocalNeuralNet.hpp"
#include "LocalNeuralNetImp.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>

#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    std::unique_ptr<LocalNeuralNet> LocalNeuralNet::make_unique(std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > input) {
        return geopm::make_unique<LocalNeuralNetImp>(input);
    }

    LocalNeuralNetImp::LocalNeuralNetImp(std::vector<std::pair<std::vector<std::vector<float> >, std::vector<float> > > input)
    {
        if (input.size() == 0u) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::size_t nlayers = input.size();

        m_layers.resize(nlayers);
        for (std::size_t idx = 0; idx < nlayers; idx++) {
            m_layers[idx] = std::make_pair(LocalNeuralNetImp::TensorTwoD(input[idx].first),
                                           LocalNeuralNetImp::TensorOneD(input[idx].second));
            if (m_layers[idx].first.get_rows() != m_layers[idx].second.get_dim()) {
                throw geopm::Exception("Incompatible dimensions for weights and biases.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            if (idx > 0 && m_layers[idx].first.get_cols() != m_layers[idx-1].first.get_rows()) {
                throw geopm::Exception("Incompatible dimensions for consecutive layers.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
        }
    }

    std::vector<float>
    LocalNeuralNetImp::model(std::vector<float> inp_vector)
    {
        LocalNeuralNetImp::TensorOneD inp(inp_vector);

        if (inp.get_dim() != m_layers[0].first.get_cols()) {
            throw geopm::Exception("Input vector dimension is incompatible with network.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        LocalNeuralNetImp::TensorOneD tmp = inp;
        std::size_t nlayers = m_layers.size();
        for (std::size_t idx = 0; idx < nlayers; idx++) {
            // Tensor operations
            tmp = m_layers[idx].first * tmp + m_layers[idx].second;

            // Apply a sigmoid on all but the last layer
            if (idx != m_layers.size() - 1) {
                tmp = tmp.sigmoid();
            }
        }
        return tmp.to_vector();
    }

    LocalNeuralNetImp::TensorOneD::TensorOneD(std::size_t dim)
    {
        set_dim(dim);
    }

    LocalNeuralNetImp::TensorOneD::TensorOneD(const TensorOneD &other)
        : m_vec(other.m_vec)
    {
    }

    LocalNeuralNetImp::TensorOneD::TensorOneD(TensorOneD &&other)
        : m_vec(std::move(other.m_vec))
    {
    }

    void LocalNeuralNetImp::TensorOneD::set_dim(std::size_t dim)
    {
        m_vec.resize(dim);
    }

    std::size_t LocalNeuralNetImp::TensorOneD::get_dim() const
    {
        return m_vec.size();
    }

    LocalNeuralNetImp::TensorOneD::TensorOneD(std::vector<float> input)
    {
        set_dim(input.size());
        std::size_t vec_size = m_vec.size();

        for (std::size_t idx = 0; idx < vec_size; ++idx) {
            m_vec[idx] = input[idx];
        }
    }

    LocalNeuralNetImp::TensorOneD LocalNeuralNetImp::TensorOneD::operator+(const LocalNeuralNetImp::TensorOneD& other)
    {
        if (get_dim() != other.get_dim()) {
            throw geopm::Exception("Adding vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        LocalNeuralNetImp::TensorOneD rval(m_vec.size());
        std::transform(m_vec.begin(), m_vec.end(), other.m_vec.begin(), rval.m_vec.begin(), std::plus<float>());

        return rval;
    }

    LocalNeuralNetImp::TensorOneD LocalNeuralNetImp::TensorOneD::operator-(const LocalNeuralNetImp::TensorOneD& other)
    {
        if (get_dim() != other.get_dim()) {
            throw geopm::Exception("Subtracting vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        LocalNeuralNetImp::TensorOneD rval(m_vec.size());
        std::transform(m_vec.begin(), m_vec.end(), other.m_vec.begin(), rval.m_vec.begin(), std::minus<float>());

        return rval;
    }


    float LocalNeuralNetImp::TensorOneD::operator*(const LocalNeuralNetImp::TensorOneD& other)
    {
        if (get_dim() != other.get_dim()) {
            throw geopm::Exception("Inner product of vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return std::inner_product(m_vec.begin(), m_vec.end(), other.m_vec.begin(), 0);
    }

    LocalNeuralNetImp::TensorOneD& LocalNeuralNetImp::TensorOneD::operator=(const LocalNeuralNetImp::TensorOneD &other)
    {
        m_vec = other.m_vec;
        return *this;
    }

    LocalNeuralNetImp::TensorOneD& LocalNeuralNetImp::TensorOneD::operator=(LocalNeuralNetImp::TensorOneD &&other)
    {
        if (&other == this) {
            return *this;
        }

        m_vec = std::move(other.m_vec);
        return *this;
    }

    float& LocalNeuralNetImp::TensorOneD::operator[] (std::size_t idx)
    {
        return m_vec.at(idx);
    }

    float LocalNeuralNetImp::TensorOneD::operator[] (std::size_t idx) const
    {
        return m_vec.at(idx);
    }

    LocalNeuralNetImp::TensorOneD LocalNeuralNetImp::TensorOneD::sigmoid() const
    {
        std::size_t vec_size = m_vec.size();
        LocalNeuralNetImp::TensorOneD rval(vec_size);
        for(std::size_t idx = 0; idx < vec_size; idx++) {
            rval[idx] = 1/(1 + expf(-(m_vec.at(idx))));
        }
        return rval;
    }

    std::vector<float> LocalNeuralNetImp::TensorOneD::to_vector() const
    {
        return m_vec;
    }

    LocalNeuralNetImp::TensorTwoD::TensorTwoD(std::size_t rows, std::size_t cols)
    {
        set_dim(rows, cols);
    }

    LocalNeuralNetImp::TensorTwoD::TensorTwoD(const LocalNeuralNetImp::TensorTwoD &other)
        : m_mat(other.m_mat)
    {
    }

    LocalNeuralNetImp::TensorTwoD::TensorTwoD(std::vector<std::vector<float> > input)
    {
        if (input.size() == 0) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::size_t rows = input.size();
        std::size_t cols = input[0].size();
        set_dim(rows, cols);
        for (std::size_t idx = 0; idx < rows; ++idx) {
            if (input[idx].size() != cols) {
                throw geopm::Exception("Attempt to load non-rectangular matrix.",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_mat[idx] = LocalNeuralNetImp::TensorOneD(input[idx]);
        }
    }

    std::size_t LocalNeuralNetImp::TensorTwoD::get_rows() const
    {
        return m_mat.size();
    }

    std::size_t LocalNeuralNetImp::TensorTwoD::get_cols() const
    {
        if (m_mat.size() == 0) {
            return 0;
        }
        return m_mat[0].get_dim();
    }

    void LocalNeuralNetImp::TensorTwoD::set_dim(std::size_t rows, std::size_t cols)
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


    LocalNeuralNetImp::TensorOneD LocalNeuralNetImp::TensorTwoD::operator*(const LocalNeuralNetImp::TensorOneD& other)
    {
        if (get_cols() != other.get_dim()) {
            throw geopm::Exception("Attempted to multiply matrix and vector with incompatible dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        LocalNeuralNetImp::TensorOneD rval(get_rows());

        for (std::size_t idx = 0; idx < get_rows(); ++idx) {
            rval[idx] = m_mat[idx] * other;
        }
        return rval;
    }

    LocalNeuralNetImp::TensorOneD& LocalNeuralNetImp::TensorTwoD::operator[](size_t idx)
    {
        return m_mat.at(idx);
    }

    LocalNeuralNetImp::TensorOneD LocalNeuralNetImp::TensorTwoD::operator[](size_t idx) const
    {
        return m_mat.at(idx);
    }

    LocalNeuralNetImp::TensorTwoD& LocalNeuralNetImp::TensorTwoD::operator=(const LocalNeuralNetImp::TensorTwoD &other)
    {
        m_mat = other.m_mat;

        return *this;
    }
}
