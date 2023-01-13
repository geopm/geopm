/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include "config.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"

#include <cmath>
#include <algorithm>
#include <functional>
#include <numeric>
#include <utility>

#include "geopm/Exception.hpp"

namespace geopm
{
    TensorOneD TensorMathImp::add(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const
    {
        if (tensor_a.get_dim() != tensor_b.get_dim()) {
            throw geopm::Exception("Adding vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<float> rval(tensor_a.get_dim());
        const auto &vec_a = tensor_a.get_data();
        const auto &vec_b = tensor_b.get_data();
        std::transform(vec_a.begin(),
                       vec_a.end(),
                       vec_b.begin(),
                       rval.begin(),
                       std::plus<float>());

        return TensorOneD(rval);
    }

    TensorOneD TensorMathImp::subtract(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const
    {
        if (tensor_a.get_dim() != tensor_b.get_dim()) {
            throw geopm::Exception("Subtracting vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<float> rval(tensor_a.get_dim());
        const auto &vec_a = tensor_a.get_data();
        const auto &vec_b = tensor_b.get_data();
        std::transform(vec_a.begin(),
                       vec_a.end(),
                       vec_b.begin(),
                       rval.begin(),
                       std::minus<float>());

        return TensorOneD(rval);
    }

    float TensorMathImp::inner_product(const TensorOneD& tensor_a, const TensorOneD& tensor_b) const
    {
        if (tensor_a.get_dim() != tensor_b.get_dim()) {
            throw geopm::Exception("Inner product of vectors of mismatched dimensions.",
                                   GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        std::vector<float> rval(tensor_a.get_dim());
        const auto &vec_a = tensor_a.get_data();
        const auto &vec_b = tensor_b.get_data();

        return std::inner_product(vec_a.begin(), vec_a.end(), vec_b.begin(), 0);
    }

    TensorOneD TensorMathImp::sigmoid(const TensorOneD tensor) const
    {
        TensorOneD rval(tensor.get_dim());
        for(std::size_t idx = 0; idx < tensor.get_dim(); idx++) {
            rval[idx] = 1/(1 + expf(-tensor[idx]));
        }
        return rval;
    }
}
