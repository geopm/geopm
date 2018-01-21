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

#ifndef MSR_HPP_INCLUDE
#define MSR_HPP_INCLUDE

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

#include "PlatformIO.hpp"


namespace geopm
{
    class IMSRIO;

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
                int domain;     /// Domain over which the MSR is shared (geopm_domain_type_e).
                int function;   /// Function which converts the bit field into an integer to be scaled (m_function_e).
                int units;      /// Scalar converts the integer output of function into units (m_units_e).
                double scalar;  /// Scale factor to convert integer output of function to SI units.
            };

            enum m_function_e {
                M_FUNCTION_SCALE,
                M_FUNCTION_LOG_HALF,        // 2.0 ^ -X
                M_FUNCTION_7_BIT_FLOAT,     // 2 ^ Y * (1.0 + Z / 4.0) : Y in [0:5), Z in [5:7)
            };

            enum m_units_e {
                M_UNITS_NONE,
                M_UNITS_SECONDS,
                M_UNITS_HZ,
                M_UNITS_WATTS,
                M_UNITS_JOULES,
                M_UNITS_CELSIUS,
            };

            IMSR() {}
            virtual ~IMSR() {}
            /// @brief Query the name of the MSR.
            /// @param msr_name [out] The name of the MSR.
            virtual std::string name(void) const = 0;
            /// @brief Program a counter to provide the MSR.
            /// @param [in] offset The offset to the programmable
            ///        counter MSR that will be read after programming
            ///        is complete (the value returned by the offest()
            ///        API).
            /// @param [in] cpu_idx Logical Linux CPU index to be
            ///        programmed.
            /// @param [in] msrio The IMSRIO object that will be used
            ///        to program the counter.
            virtual void program(uint64_t offset,
                                 int cpu_idx,
                                 IMSRIO *msrio) = 0;
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
            /// @param [out] name The name of a signal bit field.
            virtual std::string signal_name(int signal_idx) const = 0;
            /// @brief Query the name of a control bit field.
            /// @param [in] control_idx The index of the bit field in
            ///        range from to 0 to num_control() - 1.
            /// @param [out] name The name of a control bit field.
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
            /// @return The decoded signal in SI units.
            virtual double signal(int signal_idx,
                                  uint64_t field) const = 0;
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
            /// @brief The type of the domain that the MSR encodes.
            /// @return The domain type that the MSR pertains to as
            ///         defined in the geopm_domain_type_e enum
            ///         defined in the PlatformTopology.hpp header.
            virtual int domain_type(void) const = 0;
    };

    class IMSRSignal : public ISignal
    {
        public:
            struct m_signal_config_s {
                const IMSR *msr_obj;
                int cpu_idx;
                int signal_idx;
            };
            IMSRSignal() {}
            virtual ~IMSRSignal() {}
            virtual std::string name(void) const = 0;
            virtual int domain_type(void) const = 0;
            virtual int domain_idx(void) const = 0;
            virtual double sample(void) const = 0;
            /// @brief Get the number of MSRs required to generate the
            ///        signal.
            /// @return number of MSRs.
            virtual int num_msr(void) const = 0;
            virtual void offset(std::vector<uint64_t> &offset) const = 0;
            /// @brief Map 64 bits of memory storing the raw value of
            ///        an MSR that will be referenced when calculating
            ///        the signal.  This method should only be called
            ///        if the num_msr() method returns 1.
            /// @param [in] Pointer to the memory containing the raw
            ///        MSR value.
            virtual void map_field(const uint64_t *field) = 0;
            /// @brief Map a vector of pointers to the raw MSR values
            ///        that will be referenced when measuring the
            ///        signal.
            /// @param [in] field The vector of num_msr() pointers to
            ///        the raw MSR values that will be referenced when
            ///        measuring the signal.
            virtual void map_field(const std::vector<const uint64_t *> &field) = 0;
            virtual std::string log(double sample) const = 0;
    };

    class IMSRControl : public IControl
    {
        public:
            struct m_control_config_s {
                const IMSR *msr_obj;
                int cpu_idx;
                int control_idx;
            };
            IMSRControl() {}
            virtual ~IMSRControl() {}
            virtual std::string name(void) const = 0;
            virtual int domain_type(void) const = 0;
            virtual int domain_idx(void) const = 0;
            virtual void adjust(double setting) = 0;
            /// @brief Get the number of MSRs required to enforce the
            ///        control.
            /// @return number of MSRs.
            virtual int num_msr(void) const = 0;
            virtual void offset(std::vector<uint64_t> &offset) const = 0;
            virtual void mask(std::vector<uint64_t> &mask) const = 0;
            /// @brief Map 64 bits of memory storing the raw value of
            ///        an MSR that will be referenced when enforcing
            ///        the control.  This method should only be called
            ///        if the num_msr() method returns 1.
            /// @param [in] Pointer to the memory containing the raw
            ///        MSR value.
            virtual void map_field(uint64_t *field,
                                   uint64_t *mask) = 0;
            /// @brief Map a vector of pointers to the raw MSR values
            ///        that will be referenced when enforcing the control.
            /// @param [in] field The vector of num_msr() pointers to
            ///        the raw MSR values that will be referenced when
            ///        enforcing the control.
            virtual void map_field(const std::vector<uint64_t *> &field,
                                   const std::vector<uint64_t *> &mask) = 0;

    };

    class MSREncode;

    class MSR : public IMSR
    {
        public:
            /// @brief Contructor for the MSR class for fixed MSRs.
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
            /// @brief Constructor for the MSR class for programmable
            ///        counter MSRs.
            /// @param [in] msr_name The name of the programmed MSR.
            /// @param [in] signal Vector of signal name and encode
            ///        struct pairs describing all signals embedded in
            ///        the MSR once it is programmed.
            /// @param [in] program_msr Vector of IMSR objects that
            ///        are used to program the counter.
            /// @param [in] write_field_name Vector of fields in each
            ///        program_msr object that are to be configured.
            /// @param [in] prog_value Vector of values to be written
            ///        in each field to configure the MSR.
            MSR(const std::string &msr_name,
                const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
                const std::vector<const IMSR *> &prog_msr,
                const std::vector<std::string> &prog_field_name,
                const std::vector<double> &prog_value);
            /// @brief MSR class destructor.
            virtual ~MSR();
            std::string name(void) const;
            void program(uint64_t offset,
                         int cpu_idx,
                         IMSRIO *msrio);
            uint64_t offset(void) const;
            int num_signal(void) const;
            int num_control(void) const;
            std::string signal_name(int signal_idx) const;
            std::string control_name(int control_idx) const;
            int signal_index(const std::string &name) const;
            int control_index(const std::string &name) const;
            double signal(int signal_idx,
                          uint64_t field) const;
            void control(int control_idx,
                         double value,
                         uint64_t &field,
                         uint64_t &mask) const;
            int domain_type(void) const;
        protected:
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
            /// @brief Contructor for the MSRSignal class used when the
            ///        signal is determined by a single bit field in a
            ///        single MSR.
            /// @param [in] msr_obj Pointer to the MSR object
            ///        describing the MSR that contains the signal.
            /// @param [in] cpu_idx The logical Linux CPU index to
            ///        query for the MSR.
            /// @param [in] signal_idx The index of the signal within
            ///        the MSR that the class represents.
            MSRSignal(const IMSR *msr_obj,
                      int cpu_idx,
                      int signal_idx);
            MSRSignal(const IMSR *msr_obj,
                      int cpu_idx,
                      int signal_idx,
                      const std::string &name);
            /// @brief Constructor for the MSRSignal class used when
            ///        the signal is a function of several bit fields
            ///        in one or many MSRs.
            /// @param [in] config A vector of m_signal_config_s
            ///        structures describing the bit field signals
            ///        that are used to calculate a sample.  When
            ///        using this constructor the protected sample()
            ///        method that takes a vector of doubles will be
            ///        used to combine the signals.
            MSRSignal(const std::vector<IMSRSignal::m_signal_config_s> &config,
                      const std::string &name);
            virtual ~MSRSignal();
            virtual std::string name(void) const;
            int domain_type(void) const;
            int domain_idx(void) const;
            double sample(void) const;
            std::string log(double sample) const;
            int num_msr(void) const;
            void offset(std::vector<uint64_t> &offset) const;
            void map_field(const uint64_t *field);
            void map_field(const std::vector<const uint64_t *> &field);
        protected:
            /// @brief When constructed with a vector of
            ///        m_signal_config_s structures this method will
            ///        use the corresponding vector of signals to
            ///        calculate a single sample.
            /// @param [in] per_msr_signal A vector of signals
            ///        measured from each bit field described in the
            ///        contructor.
            /// @return The sample derived from the per MSR signals.
            virtual double sample(const std::vector<double> &per_msr_signal) const;

            std::vector<IMSRSignal::m_signal_config_s> m_config;
            std::string m_name;
            std::vector<const uint64_t *> m_field_ptr;
            bool m_is_field_mapped;
    };

    class MSRControl : public IMSRControl
    {
        public:
            /// @brief Contructor for the MSRControl class used when the
            ///        control is enforced with a single bit field in a
            ///        single MSR.
            /// @param [in] msr_obj Pointer to the MSR object
            ///        describing the MSR that contains the control.
            /// @param [in] cpu_idx The logical Linux CPU index to
            ///        write the MSR.
            /// @param [in] control_idx The index of the control within
            ///        the MSR that the class represents.
            MSRControl(const IMSR *msr_obj,
                       int cpu_idx,
                       int control_idx);
            MSRControl(const IMSR *msr_obj,
                       int cpu_idx,
                       int control_idx,
                       const std::string &name);
            /// @brief Constructor for the MSRControl class used when
            ///        the control is a enforced by setting several
            ///        bit fields in one or many MSRs.
            /// @param [in] config A vector of m_signal_config_s
            ///        structures describing the bit field controls
            ///        that are used to enforce the control.  When
            ///        using this constructor the protected adjust()
            ///        method that takes a double and sets a vector of
            ///        double settings will be used to derive each
            ///        control field setting.
            MSRControl(const std::vector<struct IMSRControl::m_control_config_s> &config,
                       const std::string &name);
            virtual ~MSRControl();
            virtual std::string name(void) const;
            int domain_type(void) const;
            int domain_idx(void) const;
            void adjust(double setting);
            uint64_t mask(void);
            int num_msr(void) const;
            void offset(std::vector<uint64_t> &offset) const;
            void mask(std::vector<uint64_t> &mask) const;
            void map_field(uint64_t *field, uint64_t *mask);
            void map_field(const std::vector<uint64_t *> &field, const std::vector<uint64_t *> &mask);
        protected:
            std::vector<IMSRControl::m_control_config_s> m_config;
            std::string m_name;
            std::vector<uint64_t *> m_field_ptr;
            std::vector<uint64_t *> m_mask_ptr;
            bool m_is_field_mapped;
    };

}

#endif
