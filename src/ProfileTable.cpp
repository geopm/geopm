/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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


#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <string>

#include "geopm_hash.h"
#include "Exception.hpp"
#include "ProfileTable.hpp"

#include "config.h"


namespace geopm
{
    ProfileTable::ProfileTable(size_t size, void *buffer)
        : m_buffer_size(size)
        , m_table_length(table_length(m_buffer_size))
        , m_mask(m_table_length - GEOPM_NUM_REGION_ID_PRIVATE - 1)
        , m_table((struct table_entry_s *)buffer)
        , m_key_map_lock(PTHREAD_MUTEX_INITIALIZER)
        , m_is_pshared(true)
        , m_key_map_last(m_key_map.end())
    {
        if (buffer == NULL) {
            throw Exception("ProfileTable: Buffer pointer is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (M_TABLE_DEPTH_MAX < 4) {
            throw Exception("ProfileTable: Table depth must be at least 4", GEOPM_ERROR_LOGIC, __FILE__, __LINE__);
        }
        struct table_entry_s table_init;
        memset((void *)&table_init, 0, sizeof(struct table_entry_s));
        pthread_mutexattr_t lock_attr;
        int err = pthread_mutexattr_init(&lock_attr);
        if (err) {
            throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (m_is_pshared) {
            err = pthread_mutexattr_setpshared(&lock_attr, PTHREAD_PROCESS_SHARED);
            if (err) {
                throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        for (size_t i = 0; i < m_table_length; ++i) {
            m_table[i] = table_init;
            err = pthread_mutex_init(&(m_table[i].lock), &lock_attr);
            if (err) {
                throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
    }

    ProfileTable::~ProfileTable()
    {

    }

    size_t ProfileTable::table_length(size_t buffer_size) const
    {
        size_t private_size = GEOPM_NUM_REGION_ID_PRIVATE * sizeof(struct table_entry_s);
        if (buffer_size < private_size + sizeof(struct table_entry_s)) {
            throw Exception("ProfileTable: Buffer size too small",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        size_t result = (buffer_size - private_size) / sizeof(struct table_entry_s);
        // The closest power of two small enough to fit in the buffer
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
        if (result * sizeof(struct table_entry_s) + private_size > buffer_size) {
            result /= 2;
        }
        if (result <= 0) {
            throw Exception("ProfileTable: Failing to created empty table, increase size", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        result += GEOPM_NUM_REGION_ID_PRIVATE;
        return result;
    }

    size_t ProfileTable::hash(uint64_t key) const
    {
        size_t result = 0;
        if (geopm_region_id_is_mpi(key)) {
            result = m_mask + 1;
        }
        else if (geopm_region_id_is_epoch(key)) {
            result = m_mask + 2;
        }
        else {
            result = geopm_crc32_u64(0, key) & m_mask;
        }
        return result;
    }

    void ProfileTable::insert(uint64_t key, const struct geopm_prof_message_s &value)
    {
        if (key == 0) {
            throw Exception("ProfileTable::insert(): zero is not a valid key", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        size_t table_idx = hash(key);
        int err = pthread_mutex_lock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("ProfileTable::insert(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        bool is_stored = false;
        for (size_t i = 0; !is_stored && i != M_TABLE_DEPTH_MAX; ++i) {
            if (m_table[table_idx].key[i] == 0 ||
                (m_table[table_idx].key[i] == key &&
                 !sticky(m_table[table_idx].value[i]))) {
                m_table[table_idx].key[i] = key;
                m_table[table_idx].value[i] = value;
                is_stored = true;
            }
        }
        if (!is_stored) {
            // Overwrite all sequential entry/exit pairs in array and
            // move others to head of the array, then insert new value.
            uint64_t *key_ptr = m_table[table_idx].key;
            uint64_t *key_insert_ptr = m_table[table_idx].key;
            struct geopm_prof_message_s *value_ptr = m_table[table_idx].value;
            struct geopm_prof_message_s *value_insert_ptr = m_table[table_idx].value;
            int i = 0;
            while (i < M_TABLE_DEPTH_MAX - 1) {
                if (key_ptr[0] == key_ptr[1] &&
                    value_ptr[0].region_id == value_ptr[1].region_id &&
                    value_ptr[0].progress == 0.0 &&
                    value_ptr[1].progress == 1.0) {
                    key_ptr += 2;
                    value_ptr += 2;
                    i += 2;
                }
                else {
                    *key_insert_ptr = *key_ptr;
                    ++key_insert_ptr;
                    *value_insert_ptr = *value_ptr;
                    ++value_insert_ptr;
                    ++key_ptr;
                    ++value_ptr;
                    ++i;
                }
            }
            if (i == M_TABLE_DEPTH_MAX - 1) {
                *key_insert_ptr = *key_ptr;
                *value_insert_ptr = *value_ptr;
                ++key_insert_ptr;
                ++value_insert_ptr;
            }

            if (key_insert_ptr >= m_table[table_idx].key + M_TABLE_DEPTH_MAX - 1) {
                (void) pthread_mutex_unlock(&(m_table[table_idx].lock));
                if (m_table[table_idx].value[0].region_id == GEOPM_REGION_ID_EPOCH) {
                    throw Exception("ProfileTable::insert(): controller unresponsive or epoch time interval too short.",
                                    GEOPM_ERROR_TOO_MANY_COLLISIONS, __FILE__, __LINE__);
                }
                throw Exception("ProfileTable::insert(): failed to compact table.",
                                GEOPM_ERROR_TOO_MANY_COLLISIONS, __FILE__, __LINE__);
            }
            *key_insert_ptr = key;
            *value_insert_ptr = value;
            ++key_insert_ptr;
            while (key_insert_ptr < m_table[table_idx].key + M_TABLE_DEPTH_MAX) {
                *key_insert_ptr = 0;
                ++key_insert_ptr;
            }
        }
        err = pthread_mutex_unlock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("ProfileTable::insert(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
    }

    uint64_t ProfileTable::key(const std::string &name)
    {
        uint64_t result = 0;
        int err = pthread_mutex_lock(&(m_key_map_lock));
        if (err) {
            throw Exception("ProfileTable::key(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        auto key_map_it = m_key_map.find(name);
        err = pthread_mutex_unlock(&(m_key_map_lock));
        if (err) {
            throw Exception("ProfileTable::key(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }

        if (key_map_it != m_key_map.end()) {
            result = key_map_it->second;
        }
        else {
            result = geopm_crc32_str(0, (char *)(&name.front()));
            if (!result) {
                throw Exception("ProfileTable::key(): CRC 32 hashed to zero!", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            err = pthread_mutex_lock(&(m_key_map_lock));
            if (err) {
                throw Exception("ProfileTable::key(): pthread_mutex_lock()", err, __FILE__, __LINE__);
            }
            if (m_key_set.find(result) != m_key_set.end()) {
                throw Exception("ProfileTable::key(): String hash collision", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            m_key_set.insert(result);
            m_key_map.insert(std::pair<const std::string, uint64_t>(name, result));
            m_key_map_last = m_key_map.begin();
            err = pthread_mutex_unlock(&(m_key_map_lock));
            if (err) {
                throw Exception("ProfileTable::key(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
            }
        }
        return result;
    }

    size_t ProfileTable::capacity(void) const
    {
        return m_table_length * M_TABLE_DEPTH_MAX;
    }

    size_t ProfileTable::size(void) const
    {
        int err;
        size_t result = 0;
        for (size_t table_idx = 0; table_idx < m_table_length; ++table_idx) {
            err = pthread_mutex_lock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("ProfileTable::size(): pthread_mutex_lock()", err, __FILE__, __LINE__);
            }
            for (int depth = 0; depth < M_TABLE_DEPTH_MAX && m_table[table_idx].key[depth]; ++depth) {
                ++result;
            }
            err = pthread_mutex_unlock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("ProfileTable::size(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
            }
        }
        return result;
    }

    void ProfileTable::dump(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content, size_t &length)
    {
        int err;
        length = 0;
        for (size_t table_idx = 0; table_idx < m_table_length; ++table_idx) {
            err = pthread_mutex_lock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("ProfileTable::dump(): pthread_mutex_lock()", err, __FILE__, __LINE__);
            }
            for (int depth = 0; depth != M_TABLE_DEPTH_MAX && m_table[table_idx].key[depth]; ++depth) {
                content->first = m_table[table_idx].key[depth];
                content->second = m_table[table_idx].value[depth];
                m_table[table_idx].key[depth] = 0;
                ++content;
                ++length;
            }
            err = pthread_mutex_unlock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("ProfileTable::dump(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
            }
        }
    }

    bool ProfileTable::name_fill(size_t header_offset)
    {
        bool result = false;
        size_t buffer_remain = m_buffer_size - header_offset - 1;
        char *buffer_ptr = (char *)m_table + header_offset;
        while (m_key_map_last != m_key_map.end() &&
               buffer_remain > (*m_key_map_last).first.length()) {
            strncpy(buffer_ptr, (*m_key_map_last).first.c_str(), buffer_remain);
            buffer_remain -= (*m_key_map_last).first.length() + 1;
            buffer_ptr += (*m_key_map_last).first.length() + 1;
            ++m_key_map_last;
        }
        memset(buffer_ptr, 0, buffer_remain);
        if (m_key_map_last == m_key_map.end() && buffer_remain) {
            // We are done, set last character to -1
            buffer_ptr[buffer_remain] = (char) 1;
            m_key_map_last = m_key_map.begin();
            result = true;
        }
        else {
            buffer_ptr[buffer_remain] = '\0';
        }
        return result;
    }

    bool ProfileTable::name_set(size_t header_offset, std::set<std::string> &name)
    {
        char tmp_name[NAME_MAX];
        bool result = false;
        size_t buffer_remain = m_buffer_size - header_offset - 1;
        char *buffer_ptr = (char *)m_table + header_offset;

        while (buffer_remain) {
            tmp_name[NAME_MAX - 1] = '\0';
            strncpy(tmp_name, buffer_ptr, NAME_MAX);
            if (tmp_name[NAME_MAX - 1] != '\0') {
                throw Exception("ProfileTable::name_set(): key string is too long", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (strlen(tmp_name)) {
                name.insert(std::string(tmp_name));
                buffer_remain -= strlen(tmp_name) + 1;
                buffer_ptr += strlen(tmp_name) + 1;
            }
            else {
                if (buffer_ptr[buffer_remain] == (char) 1) {
                    result = true;
                }
                buffer_remain = 0;
            }
        }
        return result;
    }

    bool ProfileTable::sticky(const struct geopm_prof_message_s &value)
    {
        bool result = false;
        if (value.progress == 0.0 || value.progress == 1.0) {
            result = true;
        }
        return result;
    }
}
