/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKBATCHCLIENT_HPP_INCLUDE
#define MOCKBATCHCLIENT_HPP_INCLUDE

#include "gmock/gmock.h"

#include "BatchClient.hpp"

class MockBatchClient : public geopm::BatchClient {
    public:
        MOCK_METHOD(std::vector<double>, read_batch, (), (override));
        MOCK_METHOD(void, write_batch, (std::vector<double> settings), (override));
        MOCK_METHOD(void, stop_batch, (), (override));
};

#endif
