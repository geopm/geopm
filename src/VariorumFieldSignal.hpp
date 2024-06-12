/*
 */

#ifndef VARIORUMFIELDSIGNAL_HPP_INCLUDE
#define VARIORUMFIELDSIGNAL_HPP_INCLUDE

#include <cstdint>
#include <cmath>

#include <string>
#include <memory>

#include "Signal.hpp"

namespace geopm
{
    /// Encapsulates conversion of Variorum bitfields to double signal
    /// values in SI units.
    /// @todo: most implementation is the same as VariorumEncode class.
    /// The hope is that this class can eventually replace the use of
    /// VariorumEncode.  The enum for the function comes from the Variorum class.
    class VariorumFieldSignal : public Signal
    {
        public:
            VariorumFieldSignal(std::shared_ptr<Signal> raw_msr,
                           int begin_bit,
                           int end_bit,
                           int function,
                           double scalar);
            VariorumFieldSignal(const VariorumFieldSignal &other) = delete;
            virtual ~VariorumFieldSignal() = default;
            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            double convert_raw_value(double val,
                                     uint64_t &last_field,
                                     int &num_overflow) const;
            /// Underlying raw Variorum that contains the field.  This
            /// should be a RawVariorumSignal in most cases but a base
            /// class pointer is used for testing and only the public
            /// interface is used.
            /// @todo: If it becomes too expensive to have another
            /// layer of indirection, this can be replaced with a
            /// pointer to the VariorumIO and an implementation similar to
            /// RawVariorumSignal.
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
