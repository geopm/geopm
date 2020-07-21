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

#ifndef MSRFIELDSIGNAL_HPP_INCLUDE
#define MSRFIELDSIGNAL_HPP_INCLUDE

#include <cstdint>
#include <cmath>

#include <string>
#include <memory>

#include "Signal.hpp"

namespace geopm
{
    /// Encapsulates conversion of MSR bitfields to double signal
    /// values in SI units.
    /// @todo: most implementation is the same as MSREncode class.
    /// The hope is that this class can eventually replace the use of
    /// MSREncode.  The enum for the function comes from the MSR class.
    class MSRFieldSignal : public Signal
    {
        public:
            MSRFieldSignal(std::shared_ptr<Signal> raw_msr,
                           int begin_bit,
                           int end_bit,
                           int function,
                           double scalar);
            MSRFieldSignal(const MSRFieldSignal &other) = delete;
            virtual ~MSRFieldSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            double convert_raw_value(double val,
                                     uint64_t &last_field,
                                     int &num_overflow) const;
            /// Underlying raw MSR that contains the field.  This
            /// should be a RawMSRSignal in most cases but a base
            /// class pointer is used for testing and only the public
            /// interface is used.
            /// @todo: If it becomes too expensive to have another
            /// layer of indirection, this can be replaced with a
            /// pointer to the MSRIO and an implementation similar to
            /// RawMSRSignal.
            std::shared_ptr<Signal> m_raw_msr;
            const int m_shift;
            const int m_num_bit;
            const uint64_t m_mask;
            const uint64_t m_subfield_max;
            const int m_function;
            const double m_scalar;
            uint64_t m_last_field;
            int m_num_overflow;
            bool m_is_batch_ready;
    };
}

#endif
