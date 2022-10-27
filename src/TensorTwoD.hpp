/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORTWOD_HPP_INCLUDE
#define TENSORTWOD_HPP_INCLUDE

#include <cstddef>

#include "geopm/json11.hpp"

#include "TensorOneD.hpp"

namespace geopm
{

    class TensorTwoD
    {
        public:
            TensorTwoD();
            TensorTwoD(std::size_t rows, std::size_t cols);
            TensorTwoD(const TensorTwoD&);
            TensorTwoD(json11::Json input);
            void set_dim(std::size_t rows, std::size_t cols);
            std::size_t get_rows() const;
            std::size_t get_cols() const;
            TensorOneD operator*(const TensorOneD&);
            TensorOneD &operator[](int idx);
            TensorOneD operator[](int idx) const;
            TensorTwoD& operator=(const TensorTwoD&);

        private:
            std::vector<TensorOneD> m_mat;
    };
}
#endif
