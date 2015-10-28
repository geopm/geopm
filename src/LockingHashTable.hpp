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

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <smmintrin.h>
#include <stdint.h>

#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <set>

#include "Exception.hpp"

#ifndef GEOPM_HASH_TABLE_DEPTH_MAX
#define GEOPM_HASH_TABLE_DEPTH_MAX 4
#endif

namespace geopm
{
    template <class type>
    class LockingHashTable
    {
        public:
            LockingHashTable(size_t size, void *buffer);
            virtual ~LockingHashTable();
            void insert(uint64_t key, const type &value);
            type find(uint64_t key);
            uint64_t key(const std::string &str);
            size_t capacity(void) const;
            void dump(std::vector<std::pair<uint64_t, type> > &contents, size_t &length);
        protected:
            struct table_entry_s {
                pthread_mutex_t lock;
                uint64_t key[GEOPM_HASH_TABLE_DEPTH_MAX];
                type value[GEOPM_HASH_TABLE_DEPTH_MAX];
            };
            size_t hash(uint64_t key) const;
            size_t table_length(size_t buffer_size) const;
            size_t m_table_length;
            uint64_t m_mask;
            struct table_entry_s *m_table;
            pthread_mutex_t m_key_map_lock;
            std::map<const std::string, uint64_t> m_key_map;
            std::set<uint64_t> m_key_set;
            bool m_is_pshared;
    };

    template <class type>
    LockingHashTable<type>::LockingHashTable(size_t size, void *buffer)
        : m_table_length(table_length(size))
        , m_mask(m_table_length - 1)
        , m_table((struct table_entry_s *)buffer)
        , m_key_map_lock(PTHREAD_MUTEX_INITIALIZER)
        , m_is_pshared(true)
    {
        if (buffer == NULL) {
            throw Exception("LockingHashTable: Buffer pointer is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        struct table_entry_s table_init = {PTHREAD_MUTEX_INITIALIZER, {0}, {0}};
        pthread_mutexattr_t lock_attr;
        int err = pthread_mutexattr_init(&lock_attr);
        if (err) {
            throw Exception("LockingHashTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (m_is_pshared) {
            err = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
            if (err) {
                throw Exception("LockingHashTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        for (size_t i = 0; i < m_table_length; ++i) {
            m_table[i] = table_init;
            err = pthread_mutex_init(&(m_table[i].lock), &lock_attr);
            if (err) {
                throw Exception("LockingHashTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    template <class type>
    LockingHashTable<type>::~LockingHashTable()
    {

    }

    template <class type>
    size_t LockingHashTable<type>::table_length(size_t buffer_size) const
    {
        // The closest power of two small enough to fit in the buffer
        size_t result = buffer_size / sizeof(struct table_entry_s);
        if (result) {
            result--;
            result |= result >> 1;
            result |= result >> 2;
            result |= result >> 4;
            result |= result >> 8;
            result |= result >> 16;
            result |= result >> 32;
            result++;
            result = result >> 1;
        }
        if (result == 0) {
            throw Exception("LockingHashTable: Failing to created empty table, increase size", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return result;
    }

    template <class type>
    size_t LockingHashTable<type>::hash(uint64_t key) const
    {
        return _mm_crc32_u64(0, key) & m_mask;
    }

    template <class type>
    void LockingHashTable<type>::insert(uint64_t key, const type &value)
    {
        if (key == 0) {
            throw Exception("LockingHashTable::insert(): zero is not a valid key", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        size_t table_idx = hash(key);
        int err = pthread_mutex_lock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("LockingHashTable::insert(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        bool is_stored = false;
        for (size_t i = 0; i < GEOPM_HASH_TABLE_DEPTH_MAX; ++i) {
            if (m_table[table_idx].key[i] == 0 ) {
                m_table[table_idx].key[i] = key;
            }
            if (m_table[table_idx].key[i] == key) {
                m_table[table_idx].value[i] = value;
                is_stored = true;
                break;
            }
        }
        err = pthread_mutex_unlock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("LockingHashTable::insert(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
        if (!is_stored) {
            throw Exception("LockingHashTable::insert(): Too many collisions", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
    }

    template <class type>
    type LockingHashTable<type>::find(uint64_t key)
    {
        if (key == 0) {
            throw Exception("LockingHashTable::find(): zero is not a valid key", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        size_t table_idx = hash(key);
        const type *result_ptr = NULL;
        int err = pthread_mutex_lock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("LockingHashTable::find(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        for (size_t i = 0; i < GEOPM_HASH_TABLE_DEPTH_MAX; ++i) {
            if (m_table[table_idx].key[i] == key) {
                result_ptr = m_table[table_idx].value + i;
                break;
            }
        }
        err = pthread_mutex_unlock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("LockingHashTable::find(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
        if (result_ptr == NULL) {
            throw Exception("LockingHashTable::find(): key not found", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        return *result_ptr;
    }

    template <class type>
    uint64_t LockingHashTable<type>::key(const std::string &str)
    {
        uint64_t result = 0;
        int err = pthread_mutex_lock(&(m_key_map_lock));
        if (err) {
            throw Exception("LockingHashTable::key(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        auto key_map_it = m_key_map.find(str);
        err = pthread_mutex_unlock(&(m_key_map_lock));
        if (err) {
            throw Exception("LockingHashTable::key(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }

        if (key_map_it != m_key_map.end()) {
            result = key_map_it->second;
        }
        else {
            size_t num_word = str.length() / 8;
            const uint64_t *ptr = (const uint64_t *)(&str.front());

            for (size_t i = 0; i < num_word; ++i) {
                result = _mm_crc32_u64(result, ptr[i]);
            }
            size_t extra = str.length() - num_word * 8;
            if (extra) {
                uint64_t last_word = 0;
                for (int i = 0; i < extra; ++i) {
                    ((char *)(&last_word))[i] = ((char *)(ptr + num_word))[i];
                }
                result = _mm_crc32_u64(result, last_word);
            }
            if (!result) {
                throw Exception("LockingHashTable::key(): CRC 32 hashed to zero!", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = pthread_mutex_lock(&(m_key_map_lock));
            if (err) {
                throw Exception("LockingHashTable::key(): pthread_mutex_lock()", err, __FILE__, __LINE__);
            }
            if (m_key_set.find(result) != m_key_set.end()) {
                throw Exception("LockingHashTable::key(): String hash collision", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_key_set.insert(result);
            m_key_map.insert(std::pair<const std::string, uint64_t>(str, result));
            err = pthread_mutex_unlock(&(m_key_map_lock));
            if (err) {
                throw Exception("LockingHashTable::key(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
            }
        }
        return result;
    }

    template <class type>
    size_t LockingHashTable<type>::capacity(void) const
    {
        return m_table_length * GEOPM_HASH_TABLE_DEPTH_MAX;
    }

    template <class type>
    void LockingHashTable<type>::dump(std::vector<std::pair<uint64_t, type> > &contents, size_t &length)
    {
        int err;
        length = 0;
        auto contents_it = contents.begin();
        for (size_t table_idx = 0; table_idx < m_table_length; ++table_idx) {
            err = pthread_mutex_lock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("LockingHashTable::dump(): pthread_mutex_lock()", err, __FILE__, __LINE__);
            }
            for (int depth = 0; depth < GEOPM_HASH_TABLE_DEPTH_MAX && m_table[table_idx].key[depth]; ++depth) {
                contents_it->first = m_table[table_idx].key[depth];
                contents_it->second = m_table[table_idx].value[depth];
                m_table[table_idx].key[depth] = 0;
                ++contents_it;
                ++length;
            }
            err = pthread_mutex_unlock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("LockingHashTable::dump(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
            }
        }
    }
}
#endif
