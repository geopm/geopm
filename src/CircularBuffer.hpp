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

#ifndef CIRCULARBUFFER_HPP_INCLUDE
#define CIRCULARBUFFER_HPP_INCLUDE

#include <stdlib.h>
#include <vector>

#include "Exception.hpp"

namespace geopm
{
    /// @brief Templated container for a circular buffer implementation.
    ///
    /// The CircularBuffer container implements a fixed size buffer. Once
    /// at capacity, any new insertions cause the oldest entry to be dropped.
    template <class type>
    class ICircularBuffer
    {
        public:
            ICircularBuffer() {}
            virtual ~ICircularBuffer() {}
            /// @brief Re-size the circular buffer.
            ///
            /// Resets the capacity of the circular buffer without
            /// modifying it's current contents.
            ///
            /// @param [in] size Requested capacity for the buffer.
            virtual void set_capacity(const unsigned int size) = 0;
            /// @brief Clears all entries from the buffer.
            virtual void clear(void) = 0;
            /// @brief Size of the buffer contents.
            ///
            /// Returns the number of items in the buffer. This
            /// value will be less than or equal to the current
            /// capacity of the buffer.
            //
            /// @return Size of the buffer contents.
            virtual int size(void) const = 0;
            /// @brief Capacity of the buffer.
            ///
            /// Returns the current size of the circular buffer at
            /// the time of the call.
            ///
            /// @return Capacity of the buffer.
            virtual int capacity(void) const = 0;
            /// @brief Insert a value into the buffer.
            ///
            /// If the buffer is not full, the new value is simply
            /// added to the buffer. It the buffer is at capacity,
            /// The head of the buffer is dropped and moved to the
            /// next oldest entry and the new value is then inserted
            /// at the end of the buffer.
            ///
            /// @param [in] value The value to be inserted.
            virtual void insert(const type value) = 0;
            /// @brief Returns a constant refernce to the value from the buffer.
            ///
            /// Accesses the contents of the circular buffer
            /// at a particular index. Valid indices range
            /// from 0 to [size-1]. Where size is the number
            /// of valid entries in the buffer. An attempt to
            /// retrieve a value for an out of bound index a
            /// geopm::Exception will be thrown with an
            /// error_value() of GEOPM_ERROR_INVALID.
            ///
            /// @param [in] index Buffer index to retrieve.
            ///
            /// @return Value from the specified buffer index.
            virtual const type& value(const unsigned int index) const = 0;
    };

    /// @brief Templated container for a circular buffer implementation.
    ///
    /// The CircularBuffer container implements a fixed size buffer. Once
    /// at capacity, any new insertions cause the oldest entry to be dropped.
    template <class type>
    class CircularBuffer : public ICircularBuffer <type>
    {
        public:
            /// @brief Constructor for the CircularBuffer template.
            ///
            /// Creates an empty circular buffer with a set capacity.
            ///
            /// @param [in] size Requested capacity for the buffer.
            CircularBuffer(unsigned int size);
            /// @brief CircularBuffer destructor, virtual
            virtual ~CircularBuffer();
            void set_capacity(const unsigned int size);
            void clear(void);
            int size(void) const;
            int capacity(void) const;
            void insert(const type value);
            const type& value(const unsigned int index) const;
        protected:
            /// @brief Vector holding the buffer data.
            std::vector<type> m_buffer;
            /// @brief Index of the current head of the buffer.
            unsigned long m_head;
            /// @brief The number of valid entries in the buffer.
            unsigned long m_count;
            /// @brief Current capacity of the buffer.
            size_t m_max_size;
    };

    template <class type>
    CircularBuffer<type>::CircularBuffer(unsigned int size)
        : m_buffer(size)
        , m_head(0)
        , m_count(0)
        , m_max_size(size)
    {

    }

    template <class type>
    CircularBuffer<type>::~CircularBuffer()
    {

    }

    template <class type>
    int CircularBuffer<type>::size() const
    {
        return m_count;
    }

    template <class type>
    int CircularBuffer<type>::capacity() const
    {
        return m_max_size;
    }

    template <class type>
    void CircularBuffer<type>::clear()
    {
        m_head = 0;
        m_count = 0;
    }

    template <class type>
    void CircularBuffer<type>::set_capacity(const unsigned int size)
    {
        if (size < m_count) {
            int size_diff = m_count - size;
            std::vector<type> temp;
            //Copy newest data into temporary vector
            for (unsigned int i = m_head + size_diff; i < ((m_head + m_count) % m_max_size); i = ((i + 1) % m_max_size)) {
                temp.push_back(m_buffer[i]);
            }
            //now re-size and swap out with tmp vector data
            m_buffer.resize(size);
            m_buffer.swap(temp);
            m_count = size;
        }
        else {
            m_buffer.resize(size);
        }
        m_head = 0;
        m_max_size = size;
    }

    template <class type>
    void CircularBuffer<type>::insert(const type value)
    {
        if (m_max_size < 1) {
            throw Exception("CircularBuffer::insert(): Cannot insert into a buffer of 0 size", GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }
        if (m_count < m_max_size) {
            m_buffer[m_count] = value;
            m_count++;
        }
        else {
            m_buffer[m_head] = value;
            m_head = ((m_head + 1) % m_max_size);
        }
    }

    template <class type>
    const type& CircularBuffer<type>::value(const unsigned int index) const
    {
        if (index >= m_count) {
            throw Exception("CircularBuffer::value(): index is out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_buffer[(m_head + index) % m_max_size];
    }
}

#endif
