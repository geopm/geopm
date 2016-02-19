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

#ifndef LOCKINGHASHTABLE_HPP_INCLUDE
#define LOCKINGHASHTABLE_HPP_INCLUDE

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdint.h>
#include <string.h>

#include <vector>
#include <algorithm>
#include <string>
#include <map>
#include <set>

#include "geopm_hash.h"
#include "Exception.hpp"

#ifndef GEOPM_HASH_TABLE_DEPTH_MAX
#define GEOPM_HASH_TABLE_DEPTH_MAX 4
#endif

namespace geopm
{
    /// @brief Templated container for multi-threaded or multi-process
    ///        producer consumer data exchange.
    ///
    /// The LockingHashTable container uses a block of virtual address
    /// space to support producer consumer data access.  The table is
    /// intended to support references which are registered once and
    /// used multiple times.  The registering of a reference requires a
    /// string name as input and provides a randomized hash of the
    /// string to an unsigned 64 bit integer key.  The key is then
    /// used for subsequent references to the templated type supported
    /// by the container.  The LockingHashTable is optimized for many
    /// writers and one reader who scans the entire table by calling
    /// LockingHashTable::dump(), however it can support other use
    /// cases as well.  The buffer that is used to store the data is
    /// provided at creation time.  This buffer can have any number of
    /// operating system memory policies applied including
    /// inter-process shared memory.  See the geopm::SharedMemory
    /// class for information on usage with POSIX inter-process shared
    /// memory.
    template <class type>
    class LockingHashTable
    {
        public:
            /// @brief Constructor for the LockingHashTable template.
            ///
            /// The memory that is used by the container is provided
            /// at construction time.  There are other ancillary data
            /// associated with the structure which are dynamic, but
            /// the templated data container is of fixed size.
            ///
            /// @param size [in] The length of the buffer in bytes.
            ///
            /// @param buffer [in] Pointer to beginning of virtual
            ///        address range used for storing the templated
            ///        data.
            LockingHashTable(size_t size, void *buffer);
            /// LockingHashTable destructor, virtual.
            virtual ~LockingHashTable();
            /// @brief Hash the name string into a random 64 bit
            ///        integer.
            ///
            /// Uses the geopm_crc32_str() function to hash the name
            /// which will modify the lower 32 bits.  The remaining 32
            /// bits may be used for other purposes in the future.
            /// Subsequent calls to hash the same string will use a
            /// string to integer std::map rather than re-hashing.
            ///
            /// @param [in] name String which is to be mapped to the
            ///        key.
            uint64_t key(const std::string &name);
            /// @brief Insert a value into the table.
            ///
            /// Once the name has been registered with a call to key()
            /// the data associated with the name can be inserted into
            /// the table by the producer using this function.  If
            /// there is already a value associated with the key then
            /// the value will be overwritten.  There is a fixed
            /// number of collisions allowed, and if too many keys
            /// hashed to the same entry in the table a
            /// geopm::Exception will be thrown with error_value() of
            /// GEOPM_ERROR_TOO_MANY_COLLISIONS.
            ///
            /// @param [in] key The value returned by key() when the
            ///        name was registered.
            ///
            /// @param [in] value Entry that is to be inserted into
            ///        the table.
            ///
            /// @return Returns the 64 bit hash used to reference the
            ///         name in other LockingHashTable methods.
            void insert(uint64_t key, const type &value);
            /// @brief Returns a copy of the data associated with the
            ///        key.
            ///
            /// Used to access a specific element of data from the
            /// table without deleting the entry.  If there is no data
            /// associated with the key or the data has been deleted
            /// by a call to dump() then a geopm::Exception is thrown
            /// with an error_value() of GEOPM_ERROR_INVALID.
            ///
            /// @param [in] key The integer returned by key() when the
            ///        name was registered.
            type find(uint64_t key);
            /// @brief Maximum number of entries the table can hold.
            ///
            /// Returns the upper bound on the number of values that
            /// can be stored in the table.  This can be used to size
            /// the content vector passed to the dump() method.  In
            /// general there will be many fewer entries into the
            /// table than the number returned by capacity() before a
            /// geopm::Exception with error_value() of
            /// GEOPM_TOO_MANY_COLLISIONS is thrown at time of
            /// insertion.
            ///
            /// @return The maximum number of entries the table can
            ///         hold.
            size_t capacity(void) const;
            /// @brief Copy all table entries into a vector and delete
            ///        all entries.
            ///
            /// This method is used by the data consumer to empty the
            /// table of all posted contents into a vector.  When the
            /// table is used in this way it serves as a temporary
            /// scratch-pad for relaying messages from the producer to
            /// the consumer.  Note that the content vector is not
            /// re-sized and it should be sized according to the value
            /// returned by capacity().  Only the first "length"
            /// elements of the vector will be written to by dump().
            ///
            /// @param [out] content The vector of key value pairs
            ///        copied out of the table.
            ///
            /// @param [out] length The number of entries copied into
            ///        the content vector.
            void dump(typename std::vector<std::pair<uint64_t, type> >::iterator content, size_t &length);
            /// @brief Called by the producer to pass names to the
            ///        consumer.
            ///
            /// When this method is called the data producer will pass
            /// the names that have been thus far been passed to key()
            /// through the buffer to the consumer who will call
            /// name_set() to receive the names.  There is an option
            /// to avoid writing to the beginning of the buffer so
            /// that it can be reserved for passing other information.
            /// If the header_offset is zero then the entire buffer is
            /// used.  This call will block until the consumer calls
            /// name_set().
            ///
            /// NOTE: The LockingHashTable cannot be used again after
            ///       a call to name_fill().
            ///
            /// @param [in] header_offset Offset in bytes to where the
            ///        name values will start in the buffer.
            bool name_fill(size_t header_offset);
            /// @brief Called by the consumer to receive the names
            ///        that hash to the keys.
            ///
            /// Through calling dump() the consumer will receive a set
            /// of integer keys.  This method enables the consumer to
            /// learn the names that can be hashed to the keys it has
            /// received.  There is an option to avoid writing to the
            /// beginning of the buffer so that it can be reserved for
            /// passing other information.  If the header_offset is
            /// zero then the entire buffer is used.  This call will
            /// block until the producer calls name_fill().
            ///
            /// NOTE: The LockingHashTable cannot be used again after
            ///       a call to name_set().
            ///
            /// @param [in] header_offset Offset in bytes to where the
            ///        name values will start in the buffer.
            ///
            /// @param [out] name Set of names read from output of the
            ///        producer's call to name_fill().
            bool name_set(size_t header_offset, std::set<std::string> &name);
        protected:
            virtual bool sticky(const type &value);
            struct table_entry_s {
                pthread_mutex_t lock;
                uint64_t key[GEOPM_HASH_TABLE_DEPTH_MAX];
                type value[GEOPM_HASH_TABLE_DEPTH_MAX];
            };
            size_t hash(uint64_t key) const;
            size_t table_length(size_t buffer_size) const;
            size_t m_buffer_size;
            size_t m_table_length;
            uint64_t m_mask;
            struct table_entry_s *m_table;
            pthread_mutex_t m_key_map_lock;
            std::map<const std::string, uint64_t> m_key_map;
            std::set<uint64_t> m_key_set;
            bool m_is_pshared;
            std::map<const std::string, uint64_t>::iterator m_key_map_last;
    };

    template <class type>
    LockingHashTable<type>::LockingHashTable(size_t size, void *buffer)
        : m_buffer_size(size)
        , m_table_length(table_length(m_buffer_size))
        , m_mask(m_table_length - 1)
        , m_table((struct table_entry_s *)buffer)
        , m_key_map_lock(PTHREAD_MUTEX_INITIALIZER)
        , m_is_pshared(true)
        , m_key_map_last(m_key_map.end())
    {
        if (buffer == NULL) {
            throw Exception("LockingHashTable: Buffer pointer is NULL", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        struct table_entry_s table_init;
        memset((void *)&table_init, 0, sizeof(struct table_entry_s));
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
        return geopm_crc32_u64(0, key) & m_mask;
    }

    template <class type>
    void LockingHashTable<type>::insert(uint64_t key, const type &value)
    {
        if (key == 0) {
            throw Exception("LockingHashTable::insert(): zero is not a valid key", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        size_t table_idx = hash(key);
        int err = pthread_mutex_lock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("LockingHashTable::insert(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        bool is_stored = false;
        for (size_t i = 0; !is_stored && i != GEOPM_HASH_TABLE_DEPTH_MAX; ++i) {
            if (m_table[table_idx].key[i] == 0 ||
                (m_table[table_idx].key[i] == key &&
                 !sticky(m_table[table_idx].value[i]))) {
                m_table[table_idx].key[i] = key;
                m_table[table_idx].value[i] = value;
                is_stored = true;
            }
        }
        err = pthread_mutex_unlock(&(m_table[table_idx].lock));
        if (err) {
            throw Exception("LockingHashTable::insert(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }
        if (!is_stored) {
            throw Exception("LockingHashTable::insert()", GEOPM_ERROR_TOO_MANY_COLLISIONS, __FILE__, __LINE__);
        }
    }

    template <class type>
    type LockingHashTable<type>::find(uint64_t key)
    {
        if (key == 0) {
            throw Exception("LockingHashTable::find(): zero is not a valid key", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
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
            throw Exception("LockingHashTable::find(): key not found", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return *result_ptr;
    }

    template <class type>
    uint64_t LockingHashTable<type>::key(const std::string &name)
    {
        uint64_t result = 0;
        int err = pthread_mutex_lock(&(m_key_map_lock));
        if (err) {
            throw Exception("LockingHashTable::key(): pthread_mutex_lock()", err, __FILE__, __LINE__);
        }
        auto key_map_it = m_key_map.find(name);
        err = pthread_mutex_unlock(&(m_key_map_lock));
        if (err) {
            throw Exception("LockingHashTable::key(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
        }

        if (key_map_it != m_key_map.end()) {
            result = key_map_it->second;
        }
        else {
            result = geopm_crc32_str(0, (char *)(&name.front()));
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
            m_key_map.insert(std::pair<const std::string, uint64_t>(name, result));
            m_key_map_last = m_key_map.begin();
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
    void LockingHashTable<type>::dump(typename std::vector<std::pair<uint64_t, type> >::iterator content, size_t &length)
    {
        int err;
        length = 0;
        for (size_t table_idx = 0; table_idx < m_table_length; ++table_idx) {
            err = pthread_mutex_lock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("LockingHashTable::dump(): pthread_mutex_lock()", err, __FILE__, __LINE__);
            }
            for (int depth = 0; depth < GEOPM_HASH_TABLE_DEPTH_MAX && m_table[table_idx].key[depth]; ++depth) {
                content->first = m_table[table_idx].key[depth];
                content->second = m_table[table_idx].value[depth];
                m_table[table_idx].key[depth] = 0;
                ++content;
                ++length;
            }
            err = pthread_mutex_unlock(&(m_table[table_idx].lock));
            if (err) {
                throw Exception("LockingHashTable::dump(): pthread_mutex_unlock()", err, __FILE__, __LINE__);
            }
        }
    }

    template <class type>
    bool LockingHashTable<type>::name_fill(size_t header_offset)
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

    template <class type>
    bool LockingHashTable<type>::name_set(size_t header_offset, std::set<std::string> &name)
    {
        char tmp_name[NAME_MAX];
        bool result = false;
        size_t buffer_remain = m_buffer_size - header_offset - 1;
        char *buffer_ptr = (char *)m_table + header_offset;

        while (buffer_remain) {
            tmp_name[NAME_MAX - 1] = '\0';
            strncpy(tmp_name, buffer_ptr, NAME_MAX);
            if (tmp_name[NAME_MAX - 1] != '\0') {
                throw Exception("LockingHashTable::name_set(): key string is too long", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
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

    template <class type>
    bool LockingHashTable<type>::sticky(const type &value)
    {
        return false;
    }
}
#endif
