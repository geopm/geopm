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

#include <stdint.h>
#include <string>
#include <vector>
#include <map>


namespace geopm
{
    /// @brief Class describing all of the encoded values within a
    /// single MSR register.  This class encodes how to access fields
    /// within an MSR, but does not hold the state of any registers.
    class IMSR
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

            IMSR() = default;
            virtual ~IMSR() = default;
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
    };

    class IMSRSignal
    {
        public:
            IMSRSignal() = default;
            virtual ~IMSRSignal() = default;
            /// @brief Get the signal parameter name.
            /// @return The name of the feature being measured.
            virtual std::string name(void) const = 0;
            /// @brief Get the type of the domain under measurement.
            /// @return One of the values from the IPlatformTopo::m_domain_e
            ///         enum described in PlatformTopo.hpp.
            virtual int domain_type(void) const = 0;
            /// @brief Get the index of the cpu in the domain under measurement.
            /// @return The index of the CPU within the set of CPUs
            ///         on the platform.
            virtual int cpu_idx(void) const = 0;
            /// @brief Get the value of the signal.
            /// @return The value of the parameter measured in SI
            ///         units.
            virtual double sample(void) = 0;
            /// @brief Gets the MSR offset for a signal.
            /// @return The MSR offset value.
            virtual uint64_t offset(void) const = 0;
            /// @brief Map 64 bits of memory storing the raw value of
            ///        an MSR that will be referenced when calculating
            ///        the signal.
            /// @param [in] field Pointer to the memory containing the raw
            ///        MSR value.
            virtual void map_field(const uint64_t *field) = 0;
    };

    class IMSRControl
    {
        public:
            IMSRControl() = default;
            virtual ~IMSRControl() = default;
            /// @brief Get the control parameter name.
            /// @return The name of the feature under control.
            virtual std::string name(void) const = 0;
            /// @brief Get the type of the domain under control.
            /// @return One of the values from the m_domain_e
            ///         enum described in PlatformTopo.hpp.
            virtual int domain_type(void) const = 0;
            /// @brief Get the index of the CPU in the domain under control.
            /// @return The index of the CPU within the set of
            ///        CPUs on the platform.
            virtual int cpu_idx(void) const = 0;
            /// @brief Set the value for the control.
            /// @param [in] setting Value in SI units of the parameter
            ///        controlled by the object.
            virtual void adjust(double setting) = 0;
            /// @brief Gets the MSR offset for the control.
            /// @param [out] offset The MSR offset value.
            virtual uint64_t offset(void) const = 0;
            /// @brief Gets the mask for the MSR that is written by
            ///        the control.
            /// @param [out] mask The write mask value.
            virtual uint64_t mask(void) const = 0;
            /// @brief Map 64 bits of memory storing the raw value of
            ///        an MSR that will be referenced when enforcing
            ///        the control.
            /// @param [in] field Pointer to the memory containing the
            ///        raw MSR value.
            /// @param [in] mask Pointer to mask that is applied when
            ///        writing value.
            virtual void map_field(uint64_t *field,
                                   uint64_t *mask) = 0;
    };

    class MSREncode;

    class MSR : public IMSR
    {
        public:
            /// @brief Constructor for the MSR class for fixed MSRs.
            /// @param [in] msr_name The name of the MSR.
            /// @param [in] offset The byte offset of the MSR.
            /// @param [in] signal Vector of signal name and encode
            ///        struct pairs describing all signals embedded in
            ///        the MSR.
            /// @param [in] control Vector of control name and encode
            /// struct pairs describing all controls embedded in the
            /// MSR.
            MSR(const std::string &msr_name,
                uint64_t offset,
                const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
                const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &control);
            /// @brief MSR class destructor.
            virtual ~MSR();
            std::string name(void) const override;
            uint64_t offset(void) const override;
            int num_signal(void) const override;
            int num_control(void) const override;
            std::string signal_name(int signal_idx) const override;
            std::string control_name(int control_idx) const override;
            int signal_index(const std::string &name) const override;
            int control_index(const std::string &name) const override;
            double signal(int signal_idx,
                          uint64_t field,
                          uint64_t &last_field,
                          uint64_t &num_overflow) const override;
            void control(int control_idx,
                         double value,
                         uint64_t &field,
                         uint64_t &mask) const override;
            uint64_t mask(int control_idx) const override;
            int domain_type(void) const override;
            int decode_function(int signal_idx) const override;
        private:
            void init(const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
                      const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &control);
            std::string m_name;
            uint64_t m_offset;
            std::vector<MSREncode *> m_signal_encode;
            std::vector<MSREncode *> m_control_encode;
            std::map<std::string, int> m_signal_map;
            std::map<std::string, int> m_control_map;
            int m_domain_type;
            const std::vector<const IMSR *> m_prog_msr;
            const std::vector<std::string> m_prog_field_name;
            const std::vector<double> m_prog_value;

    };

    class MSRSignal : public IMSRSignal
    {
        public:
            /// @brief Constructor for the MSRSignal class used when the
            ///        signal is determined by a single bit field in a
            ///        single MSR.
            /// @param [in] msr_obj Pointer to the MSR object
            ///        describing the MSR that contains the signal.
            /// @param [in] cpu_idx The logical Linux CPU index to
            ///        query for the MSR.
            /// @param [in] signal_idx The index of the signal within
            ///        the MSR that the class represents.
            MSRSignal(const IMSR &msr_obj,
                      int domain_type,
                      int cpu_idx,
                      int signal_idx);
            /// @brief Constructor for an MSRSignal corresponding to the raw
            ///        value of the entire MSR.
            MSRSignal(const IMSR &msr_obj,
                      int domain_type,
                      int cpu_idx);
            /// @brief Copy constructor.  After copying, map field
            ///        must be called again on the new MSRSignal.
            MSRSignal(const MSRSignal &other);
            MSRSignal &operator=(const MSRSignal &other) = delete;
            virtual ~MSRSignal() = default;
            virtual std::string name(void) const override;
            int domain_type(void) const override;
            int cpu_idx(void) const override;
            double sample(void) override;
            uint64_t offset(void) const override;
            void map_field(const uint64_t *field) override;
        private:
            const std::string m_name;
            const IMSR &m_msr_obj;
            const int m_domain_type;
            const int m_cpu_idx;
            const int m_signal_idx;
            const uint64_t *m_field_ptr;
            uint64_t m_field_last;
            uint64_t m_num_overflow;
            bool m_is_field_mapped;
            bool m_is_raw;
    };

    class MSRControl : public IMSRControl
    {
        public:
            /// @brief Constructor for the MSRControl class used when the
            ///        control is enforced with a single bit field in a
            ///        single MSR.
            /// @param [in] msr_obj Pointer to the MSR object
            ///        describing the MSR that contains the control.
            /// @param [in] cpu_idx The logical Linux CPU index to
            ///        write the MSR.
            /// @param [in] control_idx The index of the control within
            ///        the MSR that the class represents.
            MSRControl(const IMSR &msr_obj,
                       int domain_type,
                       int cpu_idx,
                       int control_idx);
            MSRControl(const MSRControl &other) = default;
            MSRControl &operator=(const MSRControl &other) = default;
            virtual ~MSRControl();
            virtual std::string name(void) const override;
            int domain_type(void) const override;
            int cpu_idx(void) const override;
            void adjust(double setting) override;
            uint64_t offset(void) const override;
            uint64_t mask(void) const override;
            void map_field(uint64_t *field, uint64_t *mask) override;
        private:
            const std::string m_name;
            const IMSR &m_msr_obj;
            const int m_domain_type;
            const int m_cpu_idx;
            const int m_control_idx;
            uint64_t *m_field_ptr;
            uint64_t *m_mask_ptr;
            bool m_is_field_mapped;
    };

}

#endif
