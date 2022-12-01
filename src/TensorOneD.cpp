/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorOneD.hpp"

#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>

#include "geopm/Exception.hpp"

namespace geopm
{
    TensorOneD::TensorOneD(std::size_t dim)
    {
        set_dim(dim);
    }

    TensorOneD::TensorOneD(const TensorOneD &other)
        : m_vec(other.m_vec)
    {
    }

    TensorOneD::TensorOneD(TensorOneD &&other)
        : m_vec(std::move(other.m_vec))
    {
    }

    void TensorOneD::set_dim(std::size_t dim)
    {
        m_vec.resize(dim);
    }

    std::size_t TensorOneD::get_dim() const
    {
        return m_vec.size();
    }

    TensorOneD::TensorOneD(std::vector<float> input)
    {
        if (input.size() == 0) {
            throw geopm::Exception("TensorOneD cannot be initialized with empty vector.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        m_vec = input;
    }

    TensorOneD TensorOneD::operator+(const TensorOneD& other)
    {
        if (get_dim() != other.get_dim()) {
            throw geopm::Exception("Adding vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorOneD rval(m_vec.size());
        std::transform(m_vec.begin(), m_vec.end(), other.m_vec.begin(), rval.m_vec.begin(), std::plus<float>());

        return rval;
    }

    TensorOneD TensorOneD::operator-(const TensorOneD& other)
    {
        if (get_dim() != other.get_dim()) {
            throw geopm::Exception("Subtracting vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        TensorOneD rval(m_vec.size());
        std::transform(m_vec.begin(), m_vec.end(), other.m_vec.begin(), rval.m_vec.begin(), std::minus<float>());

        return rval;
    }


    float TensorOneD::operator*(const TensorOneD& other)
    {
        if (get_dim() != other.get_dim()) {
            throw geopm::Exception("Inner product of vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        return std::inner_product(m_vec.begin(), m_vec.end(), other.m_vec.begin(), 0);
    }

    TensorOneD& TensorOneD::operator=(const TensorOneD &other)
    {
        m_vec = other.m_vec;
        return *this;
    }

    TensorOneD& TensorOneD::operator=(TensorOneD &&other)
    {
        if (&other == this) {
            return *this;
        }

        m_vec = std::move(other.m_vec);
        return *this;
    }

    float& TensorOneD::operator[] (std::size_t idx)
    {
        return m_vec.at(idx);
    }

    float TensorOneD::operator[] (std::size_t idx) const
    {
        return m_vec.at(idx);
    }

    TensorOneD TensorOneD::sigmoid() const
    {
        TensorOneD rval(m_vec.size());
        for(std::size_t idx = 0; idx < m_vec.size(); idx++) {
            rval[idx] = 1/(1 + expf(-(m_vec.at(idx))));
        }
        return rval;
    }
}
