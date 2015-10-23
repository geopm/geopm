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

#ifndef LOCKINGHASHTABLE_HPP_INCLUDE
#define LOCKINGHASHTABLE_HPP_INCLUDE

#include <vector>
#include <algorithm>
#include <smmintrin.h>
#include <limits.h>
#include <pthread.h>

#include "Exception.hpp"

namespace geopm
{
    template <class type>
    class LockingHashTable
    {
        public:
            LockingHashTable(size_t table_length, unsigned int collision_depth);
            virtual ~LockingHashTable();
            void insert(uint64_t key, const type &value);
            type find(uint64_t key);
        protected:
            size_t hash(uint64_t key);
            std::vector<std::pair<uint64_t, type> > m_table;
            std::vector<pthread_mutex_t> m_lock;
            std::vector<unsigned int> m_depth;
            uint64_t m_mask;
            size_t m_table_length;
            unsigned int m_table_depth;
    };

    template <class type>
    LockingHashTable<type>::LockingHashTable(size_t table_length, unsigned int collision_depth)
    : m_table(table_length * collision_depth)
    , m_lock(table_length)
    , m_depth(table_length)
    , m_mask(table_length - 1)
    , m_table_length(table_length)
    , m_table_depth(collision_depth)
    {
        if (table_length == 0 || collision_depth == 0) {
            throw Exception("LockingHashTable: Failing to created empty table", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (((m_mask) & table_length) != 0) {
            throw Exception("LockingHashTable: Table length must be a power of two", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        std::pair<uint64_t, type> table_fill(ULLONG_MAX, 0);
        std::fill(m_table.begin(), m_table.end(), table_fill);
        std::fill(m_lock.begin(), m_lock.end(), (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER);
        std::fill(m_depth.begin(), m_depth.end(), 0);
    }

    template <class type>
    LockingHashTable<type>::~LockingHashTable()
    {

    }

    template <class type>
    size_t LockingHashTable<type>::hash(uint64_t key)
    {
        return _mm_crc32_u64(0, key) & m_mask;
    }

    template <class type>
    void LockingHashTable<type>::insert(uint64_t key, const type &value)
    {
        int err = 0;
        int is_stored = 0;
        size_t table_idx = hash(key);

        err = pthread_mutex_lock(&(m_lock[table_idx]));
        if (err) {
            throw Exception("LockingHashTable::insert: pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        for (auto it = m_table.begin() + m_table_depth * table_idx;
                  it < m_table.begin() + m_table_depth * (table_idx + 1);
                  ++it) {
            if (it->first == ULLONG_MAX || it->first == key) {
                it->second = value;
                if (it->first == ULLONG_MAX) {
                    it->first = key;
                    ++m_depth[table_idx];
                }
                is_stored = 1;
                break;
            }
        }
        err = pthread_mutex_unlock(&(m_lock[table_idx]));
        if (err) {
            throw Exception("LockingHashTable::insert: pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
        if (!is_stored) {
            throw Exception("LockingHashTable::insert: Too many collisions", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    template <class type>
    type LockingHashTable<type>::find(uint64_t key)
    {
        size_t table_idx = hash(key);
        type *result_ptr = NULL;

        int err = pthread_mutex_lock(&(m_lock[table_idx]));
        if (err) {
            throw Exception("LockingHashTable::find: pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        for (auto it = m_table.begin() + m_table_depth * table_idx;
                  it < m_table.begin() + m_table_depth * (table_idx + 1);
                  ++it) {
            if (it->first == key) {
                result_ptr = &(it->second);
                break;
            }
        }
        err = pthread_mutex_unlock(&(m_lock[table_idx]));
        if (err) {
            throw Exception("LockingHashTable::find: pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
        if (result_ptr == NULL) {
            throw Exception("LockingHashTable::find: key not found", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return *result_ptr;
    }

}
#endif
