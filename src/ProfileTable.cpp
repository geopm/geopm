/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include "geopm_internal.h"
#include "geopm_hash.h"
#include "Exception.hpp"
#include "ProfileTable.hpp"

#include "config.h"


namespace geopm
{
    ProfileTable::ProfileTable(size_t size, void *buffer)
        : m_buffer_size(size)
        , m_table((struct table_s *)buffer)
        , m_key_map_lock(PTHREAD_MUTEX_INITIALIZER)
        , m_is_pshared(true)
        , m_key_map_last(m_key_map.end())
    {
        if (buffer == NULL) {
            throw Exception("ProfileTable: Buffer pointer is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (size < (sizeof(struct table_s) + 4 * sizeof(struct geopm_prof_message_s))) {
            throw Exception("ProfileTable: table size too small",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        // set up prof message array
        memset(buffer, 0, size);
        m_table->max_size = (m_buffer_size - sizeof(struct table_s)) / sizeof(struct geopm_prof_message_s);
        m_table->curr_size = 0;

        // set up lock
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
        err = pthread_mutex_init(&(m_table->lock), &lock_attr);
        if (err) {
            throw Exception("ProfileTable: pthread mutex initialization", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        m_table_value = (struct geopm_prof_message_s *)((char *)buffer + sizeof(struct table_s));
    }

    void ProfileTable::insert(const struct geopm_prof_message_s &value)
    {
        int err = pthread_mutex_lock(&(m_table->lock));
        if (err) {
            throw Exception("ProfileTable::insert(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        // update the progress for the same region if not an entry or exit
        bool is_inserted = false;
        if (m_table->curr_size > 0) {
            size_t curr_idx = m_table->curr_size - 1;
            if (value.region_id == m_table_value[curr_idx].region_id &&
                m_table_value[curr_idx].progress != 0.0 &&
                m_table_value[curr_idx].progress != 1.0) {

                m_table_value[curr_idx] = value;
                is_inserted = true;
            }
        }
        if (!is_inserted) {
            // check for overflow
            if (m_table->curr_size >= m_table->max_size) {
                throw Exception("ProfileTable::insert(): table overflowed.",
                                GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }

            m_table_value[m_table->curr_size] = value;
            ++m_table->curr_size;
        }
        err = pthread_mutex_unlock(&(m_table->lock));
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
            if (GEOPM_REGION_HASH_INVALID == result) {
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
        return m_table->max_size;
    }

    size_t ProfileTable::size(void) const
    {
        size_t result = 0;
        int err = pthread_mutex_lock(&(m_table->lock));
        if (err) {
            throw Exception("ProfileTable::size(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }

        result = m_table->curr_size;

        err = pthread_mutex_unlock(&(m_table->lock));
        if (err) {
            throw Exception("ProfileTable::size(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
        return result;
    }

    void ProfileTable::dump(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content, size_t &length)
    {
        int err;
        err = pthread_mutex_lock(&(m_table->lock));
        if (err) {
            throw Exception("ProfileTable::dump(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        for (size_t depth = 0; depth != m_table->curr_size; ++depth) {
            content->first = m_table_value[depth].region_id;
            content->second = m_table_value[depth];
            ++content;
        }
        length = m_table->curr_size;
        m_table->curr_size = 0;

        err = pthread_mutex_unlock(&(m_table->lock));
        if (err) {
            throw Exception("ProfileTable::dump(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
    }

    bool ProfileTable::name_fill(size_t header_offset)
    {
        bool result = false;
        size_t buffer_remain = m_buffer_size - header_offset - 1;
        char *buffer_ptr = (char *)m_table + header_offset;
        while (m_key_map_last != m_key_map.end() &&
               buffer_remain > m_key_map_last->first.length()) {
            strncpy(buffer_ptr, m_key_map_last->first.c_str(), buffer_remain);
            buffer_remain -= m_key_map_last->first.length() + 1;
            buffer_ptr += m_key_map_last->first.length() + 1;
            ++m_key_map_last;
        }
        memset(buffer_ptr, 0, buffer_remain);
        if (m_key_map_last == m_key_map.end() && buffer_remain) {
            // We are done, set last character to '\1'
            buffer_ptr[buffer_remain] = '\1';
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
        // Check if last character is '\1' to see more names remain to be passed
        bool result = (((char *)m_table)[m_buffer_size - 1] == '\1');
        size_t buffer_remain = m_buffer_size - header_offset - 1;
        char *buffer_ptr = (char *)m_table + header_offset;

        while (buffer_remain) {
            size_t name_len = strnlen(buffer_ptr, buffer_remain);
            if (name_len == buffer_remain) {
                throw Exception("ProfileTable::name_set(): buffer missing null termination", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
            if (name_len) {
                name.insert(std::string(buffer_ptr));
                buffer_remain -= name_len + 1;
                buffer_ptr += name_len + 1;
            }
            else {
                buffer_remain = 0;
            }
        }
        return result;
    }
}
