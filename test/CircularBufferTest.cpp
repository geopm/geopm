/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "CircularBuffer.hpp"
#include "Helper.hpp"

class CircularBufferTest: public :: testing :: Test
{
    protected:
        void SetUp();
        std::unique_ptr<geopm::CircularBuffer<double> > m_buffer;
};

void CircularBufferTest::SetUp()
{
    m_buffer = geopm::make_unique<geopm::CircularBuffer<double> >(5);
    m_buffer->insert(1.0);
    m_buffer->insert(2.0);
    m_buffer->insert(3.0);
}

TEST_F(CircularBufferTest, buffer_size)
{
    EXPECT_TRUE(m_buffer->size() == 3);
    m_buffer->insert(4.0);
    m_buffer->insert(5.0);
    m_buffer->insert(6.0);
    EXPECT_TRUE(m_buffer->size() == 5);
    m_buffer->clear();
    EXPECT_TRUE(m_buffer->size() == 0);
}

TEST_F(CircularBufferTest, buffer_values)
{
    EXPECT_DOUBLE_EQ(1.0, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(2.0, m_buffer->value(1));
    EXPECT_DOUBLE_EQ(3.0, m_buffer->value(2));
    m_buffer->insert(4.0);
    m_buffer->insert(5.0);
    m_buffer->insert(6.0);
    EXPECT_DOUBLE_EQ(2.0, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(3.0, m_buffer->value(1));
    EXPECT_DOUBLE_EQ(4.0, m_buffer->value(2));
    EXPECT_DOUBLE_EQ(5.0, m_buffer->value(3));
    EXPECT_DOUBLE_EQ(6.0, m_buffer->value(4));
    std::vector<double> values {2.0, 3.0, 4.0, 5.0, 6.0};
    EXPECT_EQ(values, m_buffer->make_vector());

    ASSERT_EQ(5, m_buffer->capacity());
    EXPECT_THROW(m_buffer->value(5), geopm::Exception);

    // write over old values
    m_buffer->insert(7.0);
    m_buffer->insert(8.0);
    EXPECT_DOUBLE_EQ(4.0, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(5.0, m_buffer->value(1));
    EXPECT_DOUBLE_EQ(6.0, m_buffer->value(2));
    EXPECT_DOUBLE_EQ(7.0, m_buffer->value(3));
    EXPECT_DOUBLE_EQ(8.0, m_buffer->value(4));
}

TEST_F(CircularBufferTest, buffer_capacity)
{
    EXPECT_EQ(5, m_buffer->capacity());
    m_buffer->set_capacity(10);
    EXPECT_EQ(10, m_buffer->capacity());
    m_buffer->set_capacity(2);
    EXPECT_EQ(2, m_buffer->capacity());

    // newest values maintained when capacity changes
    m_buffer->insert(1.2);
    m_buffer->insert(3.4);
    m_buffer->set_capacity(3);
    EXPECT_EQ(2, m_buffer->size());
    EXPECT_DOUBLE_EQ(1.2, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(3.4, m_buffer->value(1));
    m_buffer->insert(5.6);
    m_buffer->set_capacity(2);
    EXPECT_DOUBLE_EQ(3.4, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(5.6, m_buffer->value(1));

    // zero capacity
    m_buffer->set_capacity(0);
    EXPECT_THROW(m_buffer->insert(1.1), geopm::Exception);

    // one capacity
    m_buffer->set_capacity(1);
    m_buffer->insert(3.2);
    EXPECT_DOUBLE_EQ(3.2, m_buffer->value(0));
    m_buffer->insert(5.4);
    EXPECT_DOUBLE_EQ(5.4, m_buffer->value(0));
}

TEST_F(CircularBufferTest, make_vector_slice)
{
    // Below: Buffer is full and pointer is at the 0th position
    // in the internal buffer.
    m_buffer->insert(4.0);
    m_buffer->insert(5.0);

    std::vector<double> expected;

    expected = {1.0, 2.0, 3.0};
    EXPECT_EQ(expected, m_buffer->make_vector(0, 3));

    expected = {1.0, 2.0, 3.0, 4.0, 5.0};
    EXPECT_EQ(expected, m_buffer->make_vector(0, 5));

    expected = {2.0};
    EXPECT_EQ(expected, m_buffer->make_vector(1, 2));

    expected = {2.0, 3.0};
    EXPECT_EQ(expected, m_buffer->make_vector(1, 3));

    // Move the head of the circular buffer to position 1.
    m_buffer->insert(1.1);

    expected = {3.0, 4.0};
    EXPECT_EQ(expected, m_buffer->make_vector(1, 3));

    expected = {3.0, 4.0, 5.0};
    EXPECT_EQ(expected, m_buffer->make_vector(1, 4));

    expected = {3.0, 4.0, 5.0, 1.1};
    EXPECT_EQ(expected, m_buffer->make_vector(1, 5));

    expected = {1.1};
    EXPECT_EQ(expected, m_buffer->make_vector(4, 5));

    EXPECT_THROW(m_buffer->make_vector(5, 6), geopm::Exception);
    EXPECT_THROW(m_buffer->make_vector(5, 7), geopm::Exception);
    EXPECT_THROW(m_buffer->make_vector(0, 0), geopm::Exception);
}
