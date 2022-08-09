/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <iostream>

#ifndef TENSORONED_HPP_INCLUDE
#define TENSORONED_HPP_INCLUDE

class TensorOneD
{
    public:
        TensorOneD();
        TensorOneD(int n);
        TensorOneD(const TensorOneD&);
        ~TensorOneD();
        void set_dim(int n);
        TensorOneD operator+(const TensorOneD&);
        TensorOneD operator-(const TensorOneD&);
        float operator*(const TensorOneD&);
        TensorOneD& operator=(const TensorOneD&);
        float &operator[](int i);
        float operator[](int i) const;
        TensorOneD sigmoid();

        friend std::ostream & operator << (std::ostream &out, const TensorOneD &v);
        friend std::istream & operator >> (std::istream &in, TensorOneD &v);

    private:
        float *vec;
        int n;
        bool allocated;
};

#endif
