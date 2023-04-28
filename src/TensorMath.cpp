/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>
#include <cerrno>

#include "geopm/Exception.hpp"

namespace geopm
{
    std::shared_ptr<TensorMath> TensorMath::make_shared()
    {
        return std::make_shared<TensorMathImp>();
    }

    TensorOneD TensorMathImp::add(const TensorOneD &tensor_a, const TensorOneD &tensor_b) const
    {
        if (tensor_a.get_dim() != tensor_b.get_dim()) {
            throw Exception("TensorMathImp::" + std::string(__func__) +
                            ": Adding vectors of mismatched dimensions.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<double> rval(tensor_a.get_dim());
        const auto &vec_a = tensor_a.get_data();
        const auto &vec_b = tensor_b.get_data();
        std::transform(vec_a.begin(),
                       vec_a.end(),
                       vec_b.begin(),
                       rval.begin(),
                       std::plus<double>());

        return TensorOneD(rval);
    }

    TensorOneD TensorMathImp::subtract(const TensorOneD &tensor_a, const TensorOneD &tensor_b) const
    {
        if (tensor_a.get_dim() != tensor_b.get_dim()) {
            throw Exception("TensorMathImp::" + std::string(__func__) +
                            ": Subtracting vectors of mismatched dimensions.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<double> rval(tensor_a.get_dim());
        const auto &vec_a = tensor_a.get_data();
        const auto &vec_b = tensor_b.get_data();
        std::transform(vec_a.begin(),
                       vec_a.end(),
                       vec_b.begin(),
                       rval.begin(),
                       std::minus<double>());

        return TensorOneD(rval);
    }

    double TensorMathImp::inner_product(const TensorOneD &tensor_a, const TensorOneD &tensor_b) const
    {
        if (tensor_a.get_dim() != tensor_b.get_dim()) {
            throw Exception("TensorMathImp::" + std::string(__func__) +
                            ": Inner product of vectors of mismatched dimensions.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<double> rval(tensor_a.get_dim());
        const auto &vec_a = tensor_a.get_data();
        const auto &vec_b = tensor_b.get_data();

        return std::inner_product(vec_a.begin(), vec_a.end(), vec_b.begin(), 0);
    }

    TensorOneD TensorMathImp::sigmoid(const TensorOneD &tensor) const
    {
        TensorOneD rval(tensor.get_dim());
        for (size_t idx = 0; idx < tensor.get_dim(); ++idx) {
            // Note that a divide by zero error is impossible because denominator is 1+e^-x
            double retval = exp(-tensor[idx]);

            if (retval == HUGE_VAL) {
                errno = 0;
                rval[idx] = 0;
            }
            else {
                rval[idx] = 1/(1 + retval);
            }
        }
        return rval;
    }

    TensorOneD TensorMathImp::multiply(const TensorTwoD &tensor_a, const TensorOneD &tensor_b) const
    {
        if (tensor_a.get_cols() != tensor_b.get_dim()) {
            throw Exception("TensorMathImp::" + std::string(__func__) +
                            ": Attempted to multiply matrix and vector with incompatible dimensions.",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        const auto &MAT = tensor_a.get_data();

        TensorOneD rval(tensor_a.get_rows());

        for (size_t idx = 0; idx < tensor_a.get_rows(); ++idx) {
            rval[idx] = MAT[idx] * tensor_b;
        }
        return rval;
    }
}
