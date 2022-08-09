/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TENSORTWOD_HPP_INCLUDE
#define TENSORTWOD_HPP_INCLUDE

#include <iostream>
#include "TensorOneD.hpp"

class TensorTwoD
{
    public:
        TensorTwoD();
        TensorTwoD(int, int);
        TensorTwoD(const TensorTwoD&);
        ~TensorTwoD();
        void set_dim(int r, int c);
        TensorOneD operator*(const TensorOneD&);
        TensorOneD &operator[](int i);
        TensorOneD operator[](int i) const;
        TensorTwoD& operator=(const TensorTwoD&);
        friend std::ostream & operator << (std::ostream &out, const TensorTwoD &m);
        friend std::istream & operator >> (std::istream &in, TensorTwoD &m);

    private:
        TensorOneD *mat;
        int r, c;
        bool allocated;
};


#endif
