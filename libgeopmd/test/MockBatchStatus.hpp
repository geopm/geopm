/*
 * Copyright (c) 2015 - 2024 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKBATCHSTATUS_HPP_INCLUDE
#define MOCKBATCHSTATUS_HPP_INCLUDE

#include "gmock/gmock.h"

#include "BatchStatus.hpp"


class MockBatchStatus : public geopm::BatchStatus {
    public:
        MOCK_METHOD(void, send_message, (char msg), (override));
        MOCK_METHOD(char, receive_message, (), (override));
        MOCK_METHOD(void, receive_message, (char expect), (override));
};

#endif
