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

#include <math.h>
#include "Exception.hpp"
#include "MSRSignal.hpp"

namespace geopm
{
    MSRSignal::MSRSignal(std::vector<off_t> offset, int num_source)
        : m_num_source(num_source)
        , m_offset(offset)
        , m_lshift(offset.size())
        , m_rshift(offset.size())
        , m_mask(offset.size())
        , m_scalar(offset.size())
        , m_num_bit(offset.size())
        , m_raw_value_last(offset.size())
        , m_msr_overflow_offset(offset.size())
    {

    }

    MSRSignal::~MSRSignal()
    {

    }

    int MSRSignal::num_source(void) const
    {
        return m_num_source;
    }

    int MSRSignal::num_encoded(void) const
    {
        return m_offset.size();
    }

    void MSRSignal::decode(const std::vector<uint64_t> &encoded, std::vector<double> &decoded)
    {
        for (int signal_idx = 0; signal_idx < decoded.size(); ++signal_idx) {
            int msr_value = encoded[signal_idx];
            // Mask off bits beyond msr_size
            msr_value &= ((~0ULL) >> (64 - m_num_bit[signal_idx]));
            // Deal with register overflow
            if (msr_value < m_raw_last[signal_idx]) {
                m_overflow_offset[signal_idx] += pow(2, m_num_bit[signal_idx]);
            }
            m_raw_last[signal_idx] = msr_value;
            msr_value += m_overflow_offset[signal_idx];
            decoded[signal_idx] = (double)((((msr_value << m_lshift[signal_idx]) >> m_rshift[signal_idx]) & m_mask[signal_idx]) * m_scalar[signal_idx]);
        }
    }

    void MSRSignal::num_bit(int encoded_idx, int size)
    {
        if (m_num_bit.size() <= encoded_idx) {
            throw Exception("MSRSignal::num_bit(): Index out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_num_bit[encoded_idx] = size;
    }

    void MSRSignal::left_shift(int encoded_idx, int shift_size)
    {
        if (m_lshift.size() <= encoded_idx) {
            throw Exception("MSRSignal::left_shift(): Index out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_lshift[encoded_idx] = shift_size;
    }

    void MSRSignal::right_shift(int encoded_idx, int shift_size)
    {
        if (m_rshift.size() <= encoded_idx) {
            throw Exception("MSRSignal::right_shift(): Index out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_rshift[encoded_idx] = shift_size;
    }

    void MSRSignal::mask(int encoded_idx, uint64_t bitmask)
    {
        if (m_mask.size() <= encoded_idx) {
            throw Exception("MSRSignal::mask(): Index out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_mask[encoded_idx] = bitmask;
    }

    void MSRSignal::scalar(int encoded_idx, double scalar)
    {
        if (m_scalar.size() <= encoded_idx) {
            throw Exception("MSRSignal::scalar(): Index out of bounds", GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_scalar[encoded_idx] = scalar;
    }
}
