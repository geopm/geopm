/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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
    class CircularBuffer
    {
        public:
            CircularBuffer();
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
            std::vector<type> make_vector(void) const;
        private:
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
    CircularBuffer<type>::CircularBuffer()
        : CircularBuffer(0)
    {

    }

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

    template <class type>
    std::vector<type> CircularBuffer<type>::make_vector(void) const
    {
        std::vector<type> result(size());
        if (m_head == 0) {
            std::copy(m_buffer.begin(), m_buffer.begin() + m_count, result.begin());
        }
        else {
            std::copy(m_buffer.begin() + m_head, m_buffer.end(), result.begin());
            std::copy(m_buffer.begin(), m_buffer.begin() + m_head, result.end() - m_head);
        }
        return result;
    }
}

#endif
