/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CIRCULARBUFFER_HPP_INCLUDE
#define CIRCULARBUFFER_HPP_INCLUDE

#include <stdlib.h>

#include <vector>

#include "Exception.hpp"

namespace geopm
{
    /// @brief Templated container for a circular buffer implementation.
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
            /// @brief Re-size the circular buffer.
            ///
            /// Resets the capacity of the circular buffer without
            /// modifying its current contents.
            ///
            /// @param [in] size Requested capacity for the buffer.
            void set_capacity(const unsigned int size);
            /// @brief Clears all entries from the buffer.
            ///
            /// @details The size becomes 0, but the capacity is unchanged.
            void clear(void);
            /// @brief Size of the buffer contents.
            ///
            /// Returns the number of items in the buffer. This
            /// value will be less than or equal to the current
            /// capacity of the buffer.
            //
            /// @return Size of the buffer contents.
            int size(void) const;
            /// @brief Capacity of the buffer.
            ///
            /// Returns the current size of the circular buffer at
            /// the time of the call.
            ///
            /// @return Capacity of the buffer.
            int capacity(void) const;
            /// @brief Insert a value into the buffer.
            ///
            /// If the buffer is not full, the new value is simply
            /// added to the buffer. It the buffer is at capacity,
            /// The head of the buffer is dropped and moved to the
            /// next oldest entry and the new value is then inserted
            /// at the end of the buffer.
            ///
            /// @param [in] value The value to be inserted.
            void insert(const type value);
            /// @brief Returns a constant reference to the value from the buffer.
            ///
            /// Accesses the contents of the circular buffer
            /// at a particular index. Valid indices range
            /// from 0 to [size-1]. Where size is the number
            /// of valid entries in the buffer. An attempt to
            /// retrieve a value for an out of bound index
            /// will throw a geopm::Exception with an
            /// error_value() of GEOPM_ERROR_INVALID.
            ///
            /// @param [in] index Buffer index to retrieve.
            ///
            /// @return Value from the specified buffer index.
            const type& value(const unsigned int index) const;
            /// @brief Create a vector from the entire circular buffer contents.
            ///
            /// @return Vector containing the circular buffer contents.
            std::vector<type> make_vector(void) const;
            /// @brief Create a vector slice from the circular buffer contents.
            ///
            /// @param [in] start Start index (inclusive).
            /// @param [in] end End index (exclusive).
            ///
            /// @return Vector containing the circular buffer contents at [start, end).
            ///
            /// @throw geopm::Exception if start or end index is out of bounds
            ///        or if end index is smaller than start index
            std::vector<type> make_vector(const unsigned int start, const unsigned int end) const;
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
        if (size < m_count && m_max_size > 0) {
            int size_diff = m_count - size;
            std::vector<type> temp;
            //Copy newest data into temporary vector
            for (unsigned int i = m_head + size_diff; i != ((m_head + m_count) % m_max_size); i = ((i + 1) % m_max_size)) {
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
            m_buffer.at(m_count) = value;
            m_count++;
        }
        else {
            m_buffer.at(m_head) = value;
            m_head = ((m_head + 1) % m_max_size);
        }
    }

    template <class type>
    const type& CircularBuffer<type>::value(const unsigned int index) const
    {
        if (index >= m_count) {
            throw Exception("CircularBuffer::value(): index is out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return m_buffer.at((m_head + index) % m_max_size);
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

    template <class type>
    std::vector<type> CircularBuffer<type>::make_vector(const unsigned int idx_start, const unsigned int idx_end) const
    {
        if (idx_start >= (unsigned int)size()) {
            throw Exception("CircularBuffer::make_vector(): start is out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (idx_end > (unsigned int)size()) {
            throw Exception("CircularBuffer::make_vector(): end is out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (idx_end <= idx_start) {
            throw Exception("CircularBuffer::make_vector(): end index is smaller than start index", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }

        int slice_length = idx_end - idx_start;
        std::vector<type> result(slice_length);

        unsigned int start=(m_head + idx_start) % capacity();
        unsigned int end=(((m_head + idx_end) - 1) % capacity()) + 1;

        if(end > start) {
            std::copy(m_buffer.begin() + start, m_buffer.begin() + end, result.begin());
        }
        else {
            std::copy(m_buffer.begin() + start, m_buffer.end(), result.begin());
            std::copy(m_buffer.begin(), m_buffer.begin() + end, result.begin() + capacity() - start);
        }
        return result;
    }

}

#endif
