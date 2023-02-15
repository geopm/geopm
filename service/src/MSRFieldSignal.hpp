/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            MSRFieldSignal &operator=(const MSRFieldSignal &other) = delete;
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
