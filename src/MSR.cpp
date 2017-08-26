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

#include <sstream>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "MSR.hpp"
#include "Exception.hpp"

#define  GEOPM_IOC_MSR_BATCH _IOWR('c', 0xA2, struct MSRIO::m_msr_batch_array)

namespace geopm
{
    /// @brief Class for translating between a double precision value
    /// and the encoded 64 bit MSR value.
    class MSREncode
    {
        public:
            MSREncode(const struct IMSR::m_encode_s &msre);
            MSREncode(int begin_bit, int end_bit, double scalar);
            virtual ~MSREncode();
            double decode(uint64_t field);
            uint64_t encode(double value);
            uint64_t mask(void);
        protected:
            int m_shift;
            uint64_t m_mask;
            double m_scalar;
            double m_inverse;
    };


    MSREncode::MSREncode(const struct IMSR::m_encode_s &msre)
        : MSREncode(msre.begin_bit, msre.end_bit, msre.scalar)
    {

    }

    MSREncode::MSREncode(int begin_bit, int end_bit, double scalar)
        : m_shift(begin_bit)
        , m_mask(((1ULL << (end_bit - begin_bit)) - 1) << begin_bit)
        , m_scalar(scalar)
        , m_inverse(1.0 / scalar)
    {

    }

    MSREncode::~MSREncode()
    {

    }

    double MSREncode::decode(uint64_t field)
    {
        return m_scalar * ((field & m_mask) >> m_shift);
    }

    uint64_t MSREncode::encode(double value)
    {
        return ((uint64_t)(value * m_inverse) << m_shift) & m_mask;
    }

    uint64_t MSREncode::mask(void)
    {
        return m_mask;
    }
}
