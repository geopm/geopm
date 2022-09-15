/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORONED_HPP_INCLUDE
#define TENSORONED_HPP_INCLUDE

#include "geopm/json11.hpp"

class TensorOneD
{
    public:
        TensorOneD();
        TensorOneD(int n);
        TensorOneD(const TensorOneD&);
        TensorOneD(json11::Json input);
        void set_dim(int n);
        int get_dim();
        TensorOneD operator+(const TensorOneD&);
        TensorOneD operator-(const TensorOneD&);
        float operator*(const TensorOneD&);
        TensorOneD& operator=(const TensorOneD&);
        float &operator[](int i);
        float operator[](int i) const;
        TensorOneD sigmoid();

    private:
        std::vector<float> m_vec;
        int m_dim;
};

#endif
