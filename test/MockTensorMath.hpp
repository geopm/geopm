/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "gmock/gmock.h"

#include "TensorMath.hpp"
#include "TensorOneD.hpp"

// TODO figure out how to move this inside the class below
using geopm::TensorMath;
using geopm::TensorOneD;

class MockTensorMath : public geopm::TensorMath {
    public:
        MOCK_METHOD(TensorOneD, add, (const TensorOneD& tensor_a, const TensorOneD& tensor_b), (const, override));
        MOCK_METHOD(TensorOneD, subtract, (const TensorOneD& tensor_a, const TensorOneD& tensor_b), (const, override));
        MOCK_METHOD(float, inner_product, (const TensorOneD& tensor_a, const TensorOneD& tensor_b), (const, override));
        MOCK_METHOD(TensorOneD, sigmoid, (const TensorOneD tensor), (const, override));
};
