/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MOCKSHAREDMEMORYUSER_HPP_INCLUDE
#define MOCKSHAREDMEMORYUSER_HPP_INCLUDE

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <vector>
#include "SharedMemory.hpp"

class MockSharedMemoryUser : public geopm::ISharedMemoryUser
{
    public:
        MockSharedMemoryUser() = delete;
        MockSharedMemoryUser(size_t size) {
            m_buffer = std::vector<char>(size);
            EXPECT_CALL(*this, size())
                .WillRepeatedly(testing::Return(size));
            EXPECT_CALL(*this, pointer())
                .WillRepeatedly(testing::Return(m_buffer.data()));
            EXPECT_CALL(*this, unlink())
                .WillRepeatedly(testing::Return());
        };

        virtual ~MockSharedMemoryUser() = default;

        MOCK_CONST_METHOD0(pointer,
                           void *(void));
        MOCK_CONST_METHOD0(key,
                           std::string (void));
        MOCK_CONST_METHOD0(size,
                           size_t (void));
        MOCK_METHOD0(unlink,
                     void (void));

    protected:
        std::vector<char> m_buffer;
};
#endif
