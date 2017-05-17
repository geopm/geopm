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

#ifndef PROFILETABLE_HPP_INCLUDE
#define PROFILETABLE_HPP_INCLUDE

#include <vector>
#include <map>
#include <set>

#include "geopm_message.h"

namespace geopm
{
    /// @brief Container for multi-threaded or multi-process
    ///        producer consumer data exchange.
    ///
    /// The ProfileTable container uses a block of virtual address
    /// space to support producer consumer data access.  The table is
    /// intended to support references which are registered once and
    /// used multiple times.  The registering of a reference requires a
    /// string name as input and provides a randomized hash of the
    /// string to an unsigned 64 bit integer key.  The key is then
    /// used for subsequent references to the struct geopm_prof_message_s supported
    /// by the container.  The ProfileTable is optimized for many
    /// writers and one reader who scans the entire table by calling
    /// ProfileTable::dump(), however it can support other use
    /// cases as well.  The buffer that is used to store the data is
    /// provided at creation time.  This buffer can have any number of
    /// operating system memory policies applied including
    /// inter-process shared memory.  See the geopm::SharedMemory
    /// class for information on usage with POSIX inter-process shared
    /// memory.
    class ProfileTableBase
    {
        public:
            ProfileTableBase() {}
            virtual ~ProfileTableBase() {}
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
            virtual uint64_t key(const std::string &name) = 0;
            /// @brief Insert a value into the table.
            ///
            /// Once the name has been registered with a call to key()
            /// the data associated with the name can be inserted into
            /// the table by the producer using this function.  If
            /// there is already a value associated with the key then
            /// the value will be overwritten.  There is a fixed
            /// number of collisions allowed, and if too many keys
            /// hashed to the same entry in the table, the entry will
            /// be emptied of it's current data which will be lost.
            ///
            /// @param [in] key The value returned by key() when the
            ///        name was registered.
            ///
            /// @param [in] value Entry that is to be inserted into
            ///        the table.
            ///
            /// @return Returns the 64 bit hash used to reference the
            ///         name in other ProfileTable methods.
            virtual void insert(uint64_t key, const struct geopm_prof_message_s &value) = 0;
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
            virtual size_t capacity(void) const = 0;
            virtual size_t size(void) const = 0;
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
            virtual void dump(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content, size_t &length) = 0;
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
            /// NOTE: The ProfileTable cannot be used again after
            ///       a call to name_fill().
            ///
            /// @param [in] header_offset Offset in bytes to where the
            ///        name values will start in the buffer.
            virtual bool name_fill(size_t header_offset) = 0;
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
            /// NOTE: The ProfileTable cannot be used again after
            ///       a call to name_set().
            ///
            /// @param [in] header_offset Offset in bytes to where the
            ///        name values will start in the buffer.
            ///
            /// @param [out] name Set of names read from output of the
            ///        producer's call to name_fill().
            virtual bool name_set(size_t header_offset, std::set<std::string> &name) = 0;
    };

    class ProfileTable : public ProfileTableBase
    {
        public:
            /// @brief Constructor for the ProfileTable.
            ///
            /// The memory that is used by the container is provided
            /// at construction time.  There are other ancillary data
            /// associated with the structure which are dynamic, but
            /// the data container is of fixed size.
            ///
            /// @param size [in] The length of the buffer in bytes.
            ///
            /// @param buffer [in] Pointer to beginning of virtual
            ///        address range used for storing the data.
            ProfileTable(size_t size, void *buffer);
            /// ProfileTable destructor, virtual.
            virtual ~ProfileTable();
            uint64_t key(const std::string &name);
            void insert(uint64_t key, const struct geopm_prof_message_s &value);
            size_t capacity(void) const;
            size_t size(void) const;
            void dump(std::vector<std::pair<uint64_t, struct geopm_prof_message_s> >::iterator content, size_t &length);
            bool name_fill(size_t header_offset);
            bool name_set(size_t header_offset, std::set<std::string> &name);
        protected:
            virtual bool sticky(const struct geopm_prof_message_s &value);
            enum {
                M_TABLE_DEPTH_MAX = 16,
            };
            /// @brief structure to hold state for a single table entry.
            struct table_entry_s {
                pthread_mutex_t lock;
                uint64_t key[M_TABLE_DEPTH_MAX];
                struct geopm_prof_message_s value[M_TABLE_DEPTH_MAX];
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
}
#endif
