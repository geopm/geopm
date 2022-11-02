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

    TensorOneD::TensorOneD(json11::Json input)
    {
        if (!input.is_array()) {
            throw geopm::Exception("Neural network weights is non-array-type.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (input.array_items().empty()) {
            throw geopm::Exception("Empty array is invalid for neural network weights.\n",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        set_dim(input.array_items().size());
        std::size_t vec_size = m_vec.size();

        for (std::size_t idx = 0; idx < vec_size; ++idx) {
            if (!input[idx].is_number()) {
                throw geopm::Exception("Non-numeric type found in neural network weights.\n",
                                       GEOPM_ERROR_INVALID, __FILE__, __LINE__);
            }
            m_vec[idx] = input[idx].number_value();
        }
    }


    //TODO: Question: What happens if other and *this are the same?
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
