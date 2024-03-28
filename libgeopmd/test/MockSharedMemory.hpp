/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOCKSHAREDMEMORY_HPP_INCLUDE
#define MOCKSHAREDMEMORY_HPP_INCLUDE

#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "geopm/SharedMemory.hpp"
#include "geopm/SharedMemoryScopedLock.hpp"

class MockSharedMemory : public geopm::SharedMemory
{
    public:
        MockSharedMemory() = delete;
        MockSharedMemory(size_t size)
        {
            m_buffer = std::vector<char>(size, '\0');
            EXPECT_CALL(*this, size()).WillRepeatedly(testing::Return(size));
            EXPECT_CALL(*this, pointer())
                .WillRepeatedly(testing::Return(m_buffer.data()));
            EXPECT_CALL(*this, unlink()).WillRepeatedly(testing::Return());
        };
        virtual ~MockSharedMemory() = default;

        MOCK_METHOD(void *, pointer, (), (const, override));
        MOCK_METHOD(std::string, key, (), (const, override));
        MOCK_METHOD(size_t, size, (), (const, override));
        MOCK_METHOD(std::unique_ptr<geopm::SharedMemoryScopedLock>,
                    get_scoped_lock, (), (override));
        MOCK_METHOD(void, unlink, (), (override));
        MOCK_METHOD(void, chown, (const unsigned int gid, const unsigned int uid),
                    (const, override));

    protected:
        std::vector<char> m_buffer;
};

#endif
