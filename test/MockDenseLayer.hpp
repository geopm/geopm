/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKDENSELAYER_HPP_INCLUDE
#define MOCKDENSELAYER_HPP_INCLUDE

#include "gmock/gmock.h"
#include "DenseLayer.hpp"
#include "TensorOneD.hpp"

class MockDenseLayer : public geopm::DenseLayer
{
    public:
        MOCK_METHOD(geopm::TensorOneD, forward, (const geopm::TensorOneD &input),
                    (const override));
        MOCK_METHOD(size_t, get_input_dim, (), (const override));
        MOCK_METHOD(size_t, get_output_dim, (), (const override));
};

#endif //MOCKDenseLayer_HPP_INCLUDE
