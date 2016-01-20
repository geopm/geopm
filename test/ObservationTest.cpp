/*
 * Copyright (c) 2015, 2016, Intel Corporation
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

#include <iostream>

#include "gtest/gtest.h"
#include "geopm_error.h"
#include "Exception.hpp"
#include "Observation.hpp"

enum buff {
    HELLO,
    GOODBYE,
    ONE,
    EMPTY,
    NOTALLOCATED
};

class ObservationTest: public :: testing :: Test
{
    protected:
        void SetUp();
        geopm::Observation m_hello_obs;
};

void ObservationTest::SetUp()
{
    int i;
    int index;

    m_hello_obs.allocate_buffer(index, 10);
    printf("index 0 = %d\n",index);
    m_hello_obs.allocate_buffer(index, 10);
    printf("index 1 = %d\n",index);
    m_hello_obs.allocate_buffer(index, 10);
    printf("index 2 = %d\n",index);
    m_hello_obs.allocate_buffer(index, 10);
    printf("index 3 = %d\n",index);

    for (i = 0; i < 5; ++i) {
        m_hello_obs.insert(HELLO, (double)i);
        m_hello_obs.insert(GOODBYE, i + 5);
    }
    m_hello_obs.insert(HELLO, 0);
    m_hello_obs.insert(GOODBYE, 0);
    m_hello_obs.insert(ONE, 1);
}

TEST_F(ObservationTest, hello_mean)
{
    EXPECT_DOUBLE_EQ(1.666666666666666, m_hello_obs.mean(HELLO));
    EXPECT_DOUBLE_EQ(5.833333333333333, m_hello_obs.mean(GOODBYE));
}

TEST_F(ObservationTest, hello_median)
{
    EXPECT_DOUBLE_EQ(1.0, m_hello_obs.median(HELLO));
    EXPECT_DOUBLE_EQ(6.0, m_hello_obs.median(GOODBYE));
}

TEST_F(ObservationTest, hello_stddev)
{
    EXPECT_DOUBLE_EQ(1.632993161855452, m_hello_obs.stddev(HELLO));
    EXPECT_DOUBLE_EQ(3.188521078284832, m_hello_obs.stddev(GOODBYE));
}

TEST_F(ObservationTest, hello_max)
{
    EXPECT_DOUBLE_EQ(4.0, m_hello_obs.max(HELLO));
    EXPECT_DOUBLE_EQ(9.0, m_hello_obs.max(GOODBYE));
}

TEST_F(ObservationTest, hello_min)
{
    EXPECT_DOUBLE_EQ(0.0, m_hello_obs.min(HELLO));
    EXPECT_DOUBLE_EQ(0.0, m_hello_obs.min(GOODBYE));
}

TEST_F(ObservationTest, negative_empty)
{
    ASSERT_THROW(m_hello_obs.mean(EMPTY), geopm::Exception);
    ASSERT_THROW(m_hello_obs.median(EMPTY), geopm::Exception);
    ASSERT_THROW(m_hello_obs.stddev(EMPTY), geopm::Exception);
    ASSERT_THROW(m_hello_obs.max(EMPTY), geopm::Exception);
    ASSERT_THROW(m_hello_obs.min(EMPTY), geopm::Exception);
    ASSERT_THROW(m_hello_obs.stddev(ONE), geopm::Exception);
}

TEST_F(ObservationTest, negative_not_allocated)
{
    ASSERT_THROW(m_hello_obs.insert(NOTALLOCATED, 0.0), geopm::Exception);
    ASSERT_THROW(m_hello_obs.mean(NOTALLOCATED), geopm::Exception);
    ASSERT_THROW(m_hello_obs.median(NOTALLOCATED), geopm::Exception);
    ASSERT_THROW(m_hello_obs.stddev(NOTALLOCATED), geopm::Exception);
    ASSERT_THROW(m_hello_obs.max(NOTALLOCATED), geopm::Exception);
    ASSERT_THROW(m_hello_obs.min(NOTALLOCATED), geopm::Exception);
}
