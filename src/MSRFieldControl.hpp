/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

#ifndef MSRFIELDCONTROL_HPP_INCLUDE
#define MSRFIELDCONTROL_HPP_INCLUDE

#include <cstdint>
#include <cmath>

#include <string>
#include <memory>

#include "Control.hpp"

namespace geopm
{
    class MSRIO;

    /// Encapsulates conversion of control values in SI units to MSR
    /// bitfields.
    class MSRFieldControl : public Control
    {
        public:
            MSRFieldControl(std::shared_ptr<MSRIO> msrio,
                            int cpu,
                            uint64_t offset,
                            int begin_bit,
                            int end_bit,
                            int function,
                            double scalar);
            MSRFieldControl(const MSRFieldControl &other) = delete;
            virtual ~MSRFieldControl() = default;
            void setup_batch(void) override;
            void adjust(double value) override;
            void write(double value) override;
        private:
            uint64_t encode(double value) const;

            std::shared_ptr<MSRIO> m_msrio;
            const int m_cpu;
            const uint64_t m_offset;
            const int m_shift;
            const int m_num_bit;
            const uint64_t m_mask;
            const int m_function;
            const double m_scalar;
            const double m_inverse;
            bool m_is_batch_ready;
            int m_adjust_idx;
    };
}

#endif
