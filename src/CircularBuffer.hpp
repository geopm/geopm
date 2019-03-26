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

#include <vector>

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
            CircularBuffer() = default;
            virtual ~CircularBuffer() = default;
            /// @brief Re-size the circular buffer.
            ///
            /// Resets the capacity of the circular buffer without
            /// modifying its current contents.
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
            virtual const type& value(const unsigned int index) const = 0;
            /// @brief Create a vector from the circular buffer contents.
            ///
            /// @return Vector containing the circular buffer contents.
            virtual std::vector<type> make_vector(void) const = 0;
    };
}

#endif
