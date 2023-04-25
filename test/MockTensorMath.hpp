/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKTENSORMATH_HPP_INCLUDE
#define MOCKTENSORMATH_HPP_INCLUDE

#include "gmock/gmock.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"
#include "TensorTwoD.hpp"

class MockTensorMath : public geopm::TensorMath
{
    public:
        MOCK_METHOD(geopm::TensorOneD, add, (const geopm::TensorOneD& tensor_a,
                    const geopm::TensorOneD& tensor_b), (const, override));
        MOCK_METHOD(geopm::TensorOneD, subtract, (const geopm::TensorOneD& tensor_a,
                    const geopm::TensorOneD& tensor_b), (const, override));
        MOCK_METHOD(double, inner_product, (const geopm::TensorOneD& tensor_a,
                    const geopm::TensorOneD& tensor_b), (const, override));
        MOCK_METHOD(geopm::TensorOneD, sigmoid, (const geopm::TensorOneD& tensor),
                    (const, override));
        MOCK_METHOD(geopm::TensorOneD, multiply, (const geopm::TensorTwoD& tensor_a,
                    const geopm::TensorOneD& tensor_b), (const, override));
};

#endif
