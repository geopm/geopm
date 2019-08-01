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

#ifndef MSR_HPP_INCLUDE
#define MSR_HPP_INCLUDE

#include <cstdint>
#include <string>
#include <map>
#include <memory>
#include <vector>

namespace geopm
{
    /// @brief Class describing all of the encoded values within a
    /// single MSR register.  This class encodes how to access fields
    /// within an MSR, but does not hold the state of any registers.
    class MSR
    {
        public:
            /// @brief Structure describing a bit field in an MSR.
            struct m_encode_s {
                int begin_bit;  /// First bit of the field, inclusive.
                int end_bit;    /// Last bit of the field, exclusive.
                int domain;     /// Domain over which the MSR is shared.
                int function;   /// Function which converts the bit field into an integer to be scaled (m_function_e).
                int units;      /// Scalar converts the integer output of function into units (m_units_e).
                double scalar;  /// Scale factor to convert integer output of function to SI units.
            };

            enum m_function_e {
                M_FUNCTION_SCALE,           // Only apply scalar value (applied by all functions)
                M_FUNCTION_LOG_HALF,        // 2.0 ^ -X
                M_FUNCTION_7_BIT_FLOAT,     // 2 ^ Y * (1.0 + Z / 4.0) : Y in [0:5), Z in [5:7)
                M_FUNCTION_OVERFLOW,        // Counter that may overflow
            };

            enum m_units_e {
                M_UNITS_NONE,
                M_UNITS_SECONDS,
                M_UNITS_HERTZ,
                M_UNITS_WATTS,
                M_UNITS_JOULES,
                M_UNITS_CELSIUS,
            };

            MSR() = default;
            virtual ~MSR() = default;
            /// @brief Query the name of the MSR.
            /// @return The name of the MSR.
            virtual std::string name(void) const = 0;
            /// @brief The byte offset for the MSR.
            /// @return The 64-bit offset to the register.
            virtual uint64_t offset(void) const = 0;
            /// @brief The number of distinct signals encoded in the MSR.
            /// @return The number of contiguous bit fields in the MSR
            ///         that encode signals.
            virtual int num_signal(void) const = 0;
            /// @brief The number of distinct controls encoded in the MSR.
            /// @return The number of contiguous bit fields in the MSR
            ///         that encode controls.
            virtual int num_control(void) const = 0;
            /// @brief Query the name of a signal bit field.
            /// @param [in] signal_idx The index of the bit field in
            ///        range from to 0 to num_signal() - 1.
            /// @return The name of a signal bit field.
            virtual std::string signal_name(int signal_idx) const = 0;
            /// @brief Query the name of a control bit field.
            /// @param [in] control_idx The index of the bit field in
            ///        range from to 0 to num_control() - 1.
            /// @return The name of a control bit field.
            virtual std::string control_name(int control_idx) const = 0;
            /// @brief Query for the signal index given a name.
            /// @param [in] name The name of the signal bit field.
            /// @return Index of the signal queried unless signal name
            ///         is not found, then -1 is returned.
            virtual int signal_index(const std::string &name) const = 0;
            /// @brief Query for the control index given a name.
            /// @param [in] name The name of the control bit field.
            /// @return Index of the control queried unless control name
            ///         is not found, then -1 is returned.
            virtual int control_index(const std::string &name) const = 0;
            /// @brief Extract a signal from a raw MSR value.
            /// @param [in] signal_idx Index of the signal bit field.
            /// @param [in] field the 64-bit register value to decode.
            /// @param [in, out] last_field Previous value of the MSR.
            ///        Only relevant if the decode function is
            ///        M_FUNCTION_OVERFLOW.
            /// @param [in, out] num_overflow Number of times the
            ///        register has overflowed. Only relevant if the
            ///        decode function is M_FUNCTION_OVERFLOW.
            /// @return The decoded signal in SI units.
            virtual double signal(int signal_idx,
                                  uint64_t field,
                                  uint64_t &last_field,
                                  uint64_t &num_overflow) const = 0;
            /// @brief Set a control bit field in a raw MSR value.
            /// @param [in] control_idx Index of the control bit
            ///        field.
            /// @param [in] value The value in SI units that will be
            ///        encoded.
            /// @param [out] mask The write mask to be used when
            ///        writing the field.
            /// @param [out] field The register value to write into
            ///        the MSR.
            virtual void control(int control_idx,
                                 double value,
                                 uint64_t &field,
                                 uint64_t &mask) const = 0;
            /// @brief Get mask given a control index.
            /// @param [in] control_idx Index of the control bit
            ///        field.
            /// @return The write mask to be used when writing the
            ///         field.
            virtual uint64_t mask(int control_idx) const = 0;
            /// @brief The type of the domain that the MSR encodes.
            /// @return The domain type that the MSR pertains to as
            ///         defined in the m_domain_e enum
            ///         from the PlatformTopo.hpp header.
            virtual int domain_type(void) const = 0;
            /// @brief The function used to decode the MSR value as defined
            ///        in the m_function_e enum.
            virtual int decode_function(int signal_idx) const = 0;
            /// @brief The units for the indexed signal.
            /// @param signal_idx The index of the signal within the MSR.
            /// @return One of the MSR::m_units_e enums representing
            ///         the units of the signal
            virtual int units(int signal_idx) const = 0;
            /// @brief Convert a string to the corresponding m_function_e value
            static m_function_e string_to_function(const std::string &str);
            /// @brief Convert a string to the corresponding m_units_e value
            static m_units_e string_to_units(const std::string &str);
            static std::shared_ptr<MSR> make_shared(const std::string &msr_name,
                                                    uint64_t offset,
                                                    const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &signal,
                                                    const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &control);
            static std::unique_ptr<MSR> make_unique(const std::string &msr_name,
                                                    uint64_t offset,
                                                    const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &signal,
                                                    const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &control);
        private:
            static const std::map<std::string, m_function_e> M_FUNCTION_STRING;
            static const std::map<std::string, m_units_e> M_UNITS_STRING;
    };
}

#endif
