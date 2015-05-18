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

#include "CircularBuffer.hpp"

namespace geopm
{
    CircularBuffer::CircularBuffer(const unsigned int size)
    {
        m_max_size = size;
        m_head = 0;
        m_count = 0;
    }

    CircularBuffer::~CircularBuffer()
    {

    }

    int CircularBuffer::size() const
    {
        return m_count;
    }

    int CircularBuffer::capacity() const
    {
        return m_max_size;
    }

    void CircularBuffer::clear()
    {
        m_buffer.clear();
        m_head = 0;
        m_count = 0;
    }

    void CircularBuffer::set_capacity(const unsigned int size)
    {
        if (size < m_count) {
            int size_diff = m_count - size;
            std::vector<double> temp;
            //Copy newest data into temporary vector
            for (unsigned int i = m_head + size_diff; i < ((m_head + m_count) % m_max_size); i = ((i + 1) % m_max_size)) {
                temp.push_back(m_buffer[i]);
            }
            //now resize and swap out with tmp vector data
            m_buffer.resize(size, (double)0.0);
            m_buffer.swap(temp);
            m_count = size;
        }
        else {
            m_buffer.resize(size, 0.0);
        }
        m_head = 0;
        m_max_size = size;
    }

    void CircularBuffer::insert(const double value)
    {
        if (m_count < m_max_size) {
            m_buffer.push_back(value);
            m_count++;
        }
        else {
            m_buffer[m_head] = value;
            m_head = ((m_head + 1) % m_max_size);
        }
    }

    double CircularBuffer::value(const unsigned int index) const
    {
        return m_buffer[(m_head+index) % m_max_size];
    }
}
