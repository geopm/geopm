/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORONED_HPP_INCLUDE
#define TENSORONED_HPP_INCLUDE

#include "geopm/json11.hpp"
#include <cstddef>

namespace geopm
{
    class TensorOneD
    {
        public:
            TensorOneD();
            TensorOneD(std::size_t n);
            TensorOneD(const TensorOneD&);
            TensorOneD(json11::Json input);
            void set_dim(std::size_t n);
            std::size_t get_dim() const;
            TensorOneD operator+(const TensorOneD&);
            TensorOneD operator-(const TensorOneD&);
            float operator*(const TensorOneD&);
            TensorOneD& operator=(const TensorOneD&);
            float &operator[](std::size_t idx);
            float operator[](std::size_t idx) const;
            TensorOneD sigmoid() const;

        private:
            std::vector<float> m_vec;
    };
}
#endif
