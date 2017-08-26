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

#include "PlatformIO.hpp"


namespace geopm
{
    /// @brief Class describing all of the encoded values within a
    /// single MSR register.  This class encodes how to access fields
    /// within an MSR, but does not hold the state of any registers.
    class IMSR
    {
        public:
            /// @brief Structure describing a bit field in an MSR.
            struct m_encode_s
            {
                int begin_bit;  /// First bit of the field, inclusive.
                int end_bit;    /// Last bit of the field, exclusive.
                double scalar;  /// Scale factor to convert integer field to SI units.
            };
            IMSR() {}
            virtual ~IMSR() {}
            /// @brief Query the name of the MSR.
            /// @param msr_name [out] The name of the MSR.
            virtual void name(std::string &msr_name) const = 0;
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
            virtual void signal_name(int signal_idx,
                                     std::string &name) const = 0;
            /// @brief Query the name of a control bit field.
            /// @param [in] control_idx The index of the bit field in
            ///        range from to 0 to num_control() - 1.
            /// @param [out] name The name of a control bit field.
            virtual void control_name(int control_idx,
                                      std::string &name) const = 0;
            /// @brief Query for the signal index given a name.
            /// @param [in] name The name of the signal bit field.
            /// @return Index of the signal queried unless signal name
            ///         is not found, then -1 is returned.
            virtual int signal_index(std::string name) const = 0;
            /// @brief Query for the control index given a name.
            /// @param [in] name The name of the control bit field.
            /// @return Index of the control queried unless control name
            ///         is not found, then -1 is returned.
            virtual int control_index(std::string name) const = 0;
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
            /// @param [in] in_field The original raw MSR value that
            ///        will have one bit field be modified.
            /// @return The result of setting the control field and
            ///         leaving other bits unmodified from the
            ///         in_field parameter.
            virtual uint64_t control(int control_idx,
                                     double value,
                                     uint64_t in_field) const = 0;
            /// @brief The type of the domain that the MSR encodes.
            /// @return The domain type that the MSR pertains to as
            ///         defined in the geopm_domain_type_e enum
            ///         defined in the PlatformTopology.hpp header.
            virtual int domain_type(void) = 0;
    };

    class IMSRSignal : public ISignal
    {
        public:
            struct m_signal_config_s {
                IMSR *msr_obj;
                int cpu_idx;
                int signal_idx;
            };
            IMSRSignal() {}
            virtual ~IMSRSignal() {}
            virtual void name(std::string &name) const = 0;
            virtual int domain_type(void) const = 0;
            virtual int domain_idx(void) const = 0;
            virtual double sample(void) const = 0;
            /// @brief Get the number of MSRs required to generate the
            ///        signal.
            /// @return number of MSRs.
            virtual int num_msr(void) const = 0;
            /// @brief Map 64 bits of memory storing the raw value of
            ///        an MSR that will be referenced when calculating
            ///        the signal.  This method should only be called
            ///        if the num_msr() method returns 1.
            /// @param [in] Pointer to the memory containing the raw
            ///        MSR value.
            virtual void map(uint64_t *field) = 0;
            /// @brief Map a vector of pointers to the raw MSR values
            ///        that will be referenced when measuring the
            ///        signal.
            /// @param [in] field The vector of num_msr() pointers to
            ///        the raw MSR values that will be referenced when
            ///        measuring the signal.
            virtual void map(const std::vector<uint64_t *> &field) = 0;
    };

    class IMSRControl : public IControl
    {
        public:
            struct m_control_config_s {
                IMSR *msr_obj;
                int cpu_idx;
                int control_idx;
            };
            IMSRControl() {}
            virtual ~IMSRControl() {}
            virtual void name(std::string &name) const = 0;
            virtual int domain_type(void) const = 0;
            virtual int domain_idx(void) const = 0;
            virtual void adjust(double setting) = 0;
            /// @brief Get the number of MSRs required to enforce the
            ///        control.
            /// @return number of MSRs.
            virtual int num_msr(void) const = 0;
            /// @brief Map 64 bits of memory storing the raw value of
            ///        an MSR that will be referenced when enforcing
            ///        the control.  This method should only be called
            ///        if the num_msr() method returns 1.
            /// @param [in] Pointer to the memory containing the raw
            ///        MSR value.
            virtual void map(uint64_t *field) = 0;
            /// @brief Map a vector of pointers to the raw MSR values
            ///        that will be referenced when enforcing the control.
            /// @param [in] field The vector of num_msr() pointers to
            ///        the raw MSR values that will be referenced when
            ///        enforcing the control.
            virtual void map(const std::vector<uint64_t *> &field) = 0;
    };

    class MSR : public IMSR
    {
        public:
            /// @brief Contructor for the MSR class.
            /// @param [in] offset The byte offset of the MSR.
            /// @param [in] msr_name The name of the MSR.
            /// @param [in] signal Vector of signal name and encode
            ///        struct pairs describing all signals embedded in
            ///        the MSR.
            /// @param [in] control Vector of control name and encode
            /// struct pairs describing all controls embedded in the
            /// MSR.
            MSR(uint64_t offset,
                const std::string &msr_name,
                const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &signal,
                const std::vector<std::pair<std::string, struct IMSR::m_encode_s> > &control);
            /// @brief MSR class destructor.
            virtual ~MSR();
            void name(std::string &msr_name) const;
            uint64_t offset(void) const;
            int num_signal(void) const;
            int num_control(void) const;
            void signal_name(int signal_idx,
                             std::string &name) const;
            void control_name(int control_idx,
                              std::string &name) const;
            int signal_index(std::string name) const;
            int control_index(std::string name) const;
            double signal(int signal_idx,
                          uint64_t field) const;
            uint64_t control(int control_idx,
                             double value,
                             uint64_t in_field) const;
            int domain_type(void);
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
            /// @brief Constructor for the MSRSignal class used when
            ///        the signal is a function of several bit fields
            ///        in one or many MSRs.
            /// @param [in] config A vector of m_signal_config_s
            ///        structures describing the bit field signals
            ///        that are used to calculate a sample.  When
            ///        using this constructor the protected sample()
            ///        method that takes a vector of doubles will be
            ///        used to combine the signals.
            MSRSignal(std::vector<IMSRSignal::m_signal_config_s> config);
            virtual ~MSRSignal();
            void name(std::string &name) const;
            int domain_type(void) const;
            int domain_idx(void) const;
            double sample(void) const;
            int num_msr(void) const;
            void map(uint64_t *field);
            void map(std::vector<uint64_t *> field);
        protected:
            /// @brief When constructed with a vector of
            ///        m_signal_config_s structures this method will
            ///        use the corresponding vector of signals to
            ///        calculate a single sample.
            /// @param [in] per_msr_signal A vector of signals
            ///        measured from each bit field described in the
            ///        contructor.
            /// @return The sample derived from the per MSR signals.
            virtual double sample(const std::vector<double> &per_msr_signal);
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
            MSRControl(std::vector<struct IMSRControl::m_control_config_s> config);
            virtual ~MSRControl() {}
            void name(std::string &name) const;
            int domain_type(void) const;
            int domain_idx(void) const;
            void adjust(double setting);
            int num_msr(void) const;
            void map(uint64_t *field);
            void map(const std::vector<uint64_t *> &field);
        protected:
            /// @brief When constructed with a vector of
            ///        m_control_config_s structures this method will
            ///        create the corresponding vector of settings for
            ///        each control field given the desired setting
            ///        for the control.
            /// @param [in] setting The setting to enforce for the
            ///        control.
            /// @param [out] per_msr_setting A vector of settings for
            ///        each bit field described in the contructor
            ///        which enforce the setting provided.
            virtual void adjust(double setting,
                                std::vector<double> &per_msr_setting) const;
    };

}

#endif
