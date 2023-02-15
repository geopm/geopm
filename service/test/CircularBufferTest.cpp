/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <memory>
#include <vector>

#include "gtest/gtest.h"
#include "geopm/CircularBuffer.hpp"
#include "geopm/Helper.hpp"

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
    // this one overflows the capacity, discards the oldest value 1.0
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

TEST_F(CircularBufferTest, buffer_values_negative_indices)
{
    m_buffer->insert(4.0);
    m_buffer->insert(5.0);
    EXPECT_DOUBLE_EQ(1.0, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(2.0, m_buffer->value(1));
    EXPECT_DOUBLE_EQ(3.0, m_buffer->value(2));
    EXPECT_DOUBLE_EQ(4.0, m_buffer->value(3));
    EXPECT_DOUBLE_EQ(5.0, m_buffer->value(4));
    //
    EXPECT_DOUBLE_EQ(5.0, m_buffer->value(-1));
    EXPECT_DOUBLE_EQ(4.0, m_buffer->value(-2));
    EXPECT_DOUBLE_EQ(3.0, m_buffer->value(-3));
    EXPECT_DOUBLE_EQ(2.0, m_buffer->value(-4));
    EXPECT_DOUBLE_EQ(1.0, m_buffer->value(-5));
    // overflows the capacity, writes over the oldest values
    // shifts the rest of the values to the m_head
    m_buffer->insert(10.0);
    m_buffer->insert(11.0);
    m_buffer->insert(12.0);
    EXPECT_DOUBLE_EQ(4.0, m_buffer->value(0));
    EXPECT_DOUBLE_EQ(5.0, m_buffer->value(1));
    EXPECT_DOUBLE_EQ(10.0, m_buffer->value(2));
    EXPECT_DOUBLE_EQ(11.0, m_buffer->value(3));
    EXPECT_DOUBLE_EQ(12.0, m_buffer->value(4));
    //
    EXPECT_DOUBLE_EQ(12.0, m_buffer->value(-1));
    EXPECT_DOUBLE_EQ(11.0, m_buffer->value(-2));
    EXPECT_DOUBLE_EQ(10.0, m_buffer->value(-3));
    EXPECT_DOUBLE_EQ(5.0, m_buffer->value(-4));
    EXPECT_DOUBLE_EQ(4.0, m_buffer->value(-5));
    // test invalid indices
    ASSERT_EQ(5, m_buffer->capacity());
    EXPECT_THROW(m_buffer->value(5), geopm::Exception);
    EXPECT_THROW(m_buffer->value(6), geopm::Exception);
    EXPECT_THROW(m_buffer->value(7), geopm::Exception);
    //
    EXPECT_THROW(m_buffer->value(-6), geopm::Exception);
    EXPECT_THROW(m_buffer->value(-7), geopm::Exception);
    EXPECT_THROW(m_buffer->value(-8), geopm::Exception);
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
