/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"

#include "geopm/Exception.hpp"

namespace geopm
{
    TensorOneD::TensorOneD()
        : TensorOneD(0)
    {
    }

    TensorOneD::TensorOneD(size_t dim)
        : TensorOneD(std::vector<float>(dim))
    {
    }

    TensorOneD::TensorOneD(const TensorOneD &other)
        : TensorOneD(other.m_vec)
    {
    }

    TensorOneD::TensorOneD(TensorOneD &&other)
        : TensorOneD(std::move(other.m_vec))
    {
    }

    TensorOneD::TensorOneD(std::vector<float> input)
        : TensorOneD(input, std::make_shared<TensorMathImp>())
    {
    }

    TensorOneD::TensorOneD(std::vector<float> input, std::shared_ptr<TensorMath> math)
    {
        m_vec = input;
        m_math = math;
    }

    void TensorOneD::set_dim(size_t dim)
    {
        m_vec.resize(dim, 0);
    }

    size_t TensorOneD::get_dim() const
    {
        return m_vec.size();
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

    bool TensorOneD::operator==(const TensorOneD &other) const
    {
        return m_vec == other.m_vec;
    }

    float& TensorOneD::operator[] (size_t idx)
    {
        return m_vec.at(idx);
    }

    float TensorOneD::operator[] (size_t idx) const
    {
        return m_vec.at(idx);
    }

    const std::vector<float> &TensorOneD::get_data() const
    {
        return m_vec;
    }


    // TODO put these in the right places
    TensorOneD TensorOneD::operator+(const TensorOneD &other) const {
        return m_math->add(*this, other);
    }
    TensorOneD TensorOneD::operator-(const TensorOneD &other) const {
        return m_math->subtract(*this, other);
    }
    float TensorOneD::operator*(const TensorOneD &other) const {
        return m_math->inner_product(*this, other);
    }
    TensorOneD TensorOneD::sigmoid() const {
        return m_math->sigmoid(*this);
    }
}
