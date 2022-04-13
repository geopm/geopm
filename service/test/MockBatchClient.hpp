/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSBATCHCLIENT_HPP_INCLUDE
#define MOCKSBATCHCLIENT_HPP_INCLUDE

#include "gmock/gmock.h"

#include "BatchClient.hpp"

class MockBatchClient : public geopm::BatchClient {
    public:
        MOCK_METHOD(std::vector<double>, read_batch, (), (override));
        MOCK_METHOD(void, write_batch, (std::vector<double> settings), (override));
        MOCK_METHOD(void, stop_batch, (), (override));
};

#endif
