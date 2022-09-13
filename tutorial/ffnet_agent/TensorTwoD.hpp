/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORTWOD_HPP_INCLUDE
#define TENSORTWOD_HPP_INCLUDE

#include "geopm/json11.hpp"

#include "TensorOneD.hpp"

class TensorTwoD
{
    public:
        TensorTwoD();
        TensorTwoD(int, int);
        TensorTwoD(const TensorTwoD&);
        TensorTwoD(json11::Json input);
        void set_dim(int r, int c);
        int get_rows();
        int get_cols();
        TensorOneD operator*(const TensorOneD&);
        TensorOneD &operator[](int i);
        TensorOneD operator[](int i) const;
        TensorTwoD& operator=(const TensorTwoD&);

    private:
        std::vector<TensorOneD> mat;
        int r, c;
};

#endif
