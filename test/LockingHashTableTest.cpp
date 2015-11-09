/*
 * Copyright (c) 2015, Intel Corporation
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

#include <stdlib.h>
#include "gtest/gtest.h"
#include "LockingHashTable.hpp"

class LockingHashTableTest: public :: testing :: Test
{
    public:
        LockingHashTableTest();
        virtual ~LockingHashTableTest();
        void overfill_small(void);
    protected:
        size_t m_size;
        size_t m_small_size;
        char m_ptr[2048];
        char m_small_ptr[512];
        geopm::LockingHashTable<double> *m_table;
        geopm::LockingHashTable<double> *m_table_small;
};

LockingHashTableTest::LockingHashTableTest()
    : m_size(sizeof(m_ptr))
    , m_small_size(sizeof(m_small_ptr))
{
    m_table = new geopm::LockingHashTable<double>(m_size, (void *)m_ptr);
    m_table_small = new geopm::LockingHashTable<double>(m_small_size, (void *)m_small_ptr);
}

LockingHashTableTest::~LockingHashTableTest()
{
    delete m_table_small;
    delete m_table;
}

void LockingHashTableTest::overfill_small(void)
{
    for (size_t i = 1; i <= m_table_small->capacity() + 1; ++i) {
        m_table_small->insert(i, (double)i);
    }
}

TEST_F(LockingHashTableTest, hello)
{
    m_table->insert(1234, 1.234);
    EXPECT_EQ(1.234, m_table->find(1234));
    m_table->insert(5678, 5.678);
    EXPECT_EQ(1.234, m_table->find(1234));
    EXPECT_EQ(5.678, m_table->find(5678));
    m_table->insert(5678, 9.876);
    EXPECT_EQ(9.876, m_table->find(5678));
    EXPECT_THROW(m_table->find(0), geopm::Exception);
    EXPECT_THROW(geopm::LockingHashTable<double>(0,NULL), geopm::Exception);
    uint64_t tmp[128];
    EXPECT_THROW(geopm::LockingHashTable<double>(1,tmp), geopm::Exception);
    uint64_t key0 = m_table->key("hello");
    uint64_t key1 = m_table->key("hello1");
    uint64_t key2 = m_table->key("hello");
    EXPECT_NE(key0, key1);
    EXPECT_EQ(key0, key2);
    m_table->insert(key0, 1234.5);
    EXPECT_EQ(1234.5, m_table->find(key0));
    EXPECT_THROW(overfill_small(), geopm::Exception);
    std::vector<std::pair<uint64_t, double> > contents(3);
    size_t length;
    m_table->dump(contents.begin(), length);
    EXPECT_EQ(3, length);
    for (int i = 0; i < 3; ++i) {
        if (contents[i].first == 1234) {
            EXPECT_EQ(1.234, contents[i].second);
        }
        else if (contents[i].first == 5678) {
            EXPECT_EQ(9.876, contents[i].second);
        }
        else if (contents[i].first == key0) {
            EXPECT_EQ(1234.5, contents[i].second);
        }
        else {
            EXPECT_TRUE(false);
        }
    }
}
