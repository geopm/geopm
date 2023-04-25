/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKLOCALNEURALNET_HPP_INCLUDE
#define MOCKLOCALNEURALNET_HPP_INCLUDE

#include "gmock/gmock.h"
#include "LocalNeuralNet.hpp"

class MockLocalNeuralNet : public geopm::LocalNeuralNet
{
    public:
        MOCK_METHOD(geopm::TensorOneD, forward, (const geopm::TensorOneD &input),
                    (const override));
        MOCK_METHOD(size_t, get_input_dim, (), (const override));
        MOCK_METHOD(size_t, get_output_dim, (), (const override));
};

#endif //MOCKLOCALNEURALNET_HPP_INCLUDE
