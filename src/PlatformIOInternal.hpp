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

#ifndef PLATFORMIOINTERNAL_HPP_INCLUDE
#define PLATFORMIOINTERNAL_HPP_INCLUDE

namespace geopm
{
    class IMSR;
    class IMSRIO;

    class PlatformIO : public IPlatformIO
    {
        public:
            /// @brief Constructor for the PlatformIO class.
            PlatformIO();
            /// @brief Virtual destructor for the PlatformIO class.
            virtual ~PlatformIO();
            int push_signal(const std::string &signal_name,
                            int domain_type,
                            int domain_idx);
            int push_control(const std::string &control_name,
                             int domain_type,
                             int domain_idx);
            void clear(void);
            void sample(std::vector<double> &signal);
            void adjust(const std::vector<double> &setting);
       protected:
            enum m_cpuid_e {
                M_CPUID_SNB = 0x62D,
                M_CPUID_IVT = 0x63E,
                M_CPUID_HSX = 0x63F,
                M_CPUID_BDX = 0x64F,
                M_CPUID_KNL = 0x657,
            };

            virtual int cpuid(void);
            /// @brief Register all signals and controls for the MSR
            ///        interface.
            virtual void init_msr(void);
            /// @brief Register a single MSR field as a signal. This
            ///        is called by init_msr().
            /// @param [in] signal_name Compound signal name of form
            ///        "msr_name:field_name" where msr_name is the
            ///        name of the MSR and the field_name is the name
            ///        of the signal field held in the MSR.
            void register_msr_signal(const std::string &signal_name);
            /// @brief Register a signal for the MSR interface.  This
            ///        is called by init_msr().
            /// @param [in] signal_name The name of the signal as it
            ///        is requested by the push_signal() method.
            /// @param [in] msr_name Vector of MSR names that are used
            ///        to construct the signal.
            /// @param [in] field_name Vector of field names that
            ///        are read from each corresponding MSR in the
            ///        msr_name vector.
            void register_msr_signal(const std::string &signal_name,
                                     const std::vector<std::string> &msr_name,
                                     const std::vector<std::string> &field_name);
            /// @brief Register a single MSR field as a control. This
            ///        is called by init_msr().
            /// @param [in] signal_name Compound control name of form
            ///        "msr_name:field_name" where msr_name is the
            ///        name of the MSR and the field_name is the name
            ///        of the control field held in the MSR.
            void register_msr_control(const std::string &control_name);
            /// @brief Register a contol for the MSR interface.  This
            ///        is called by init_msr().
            /// @param [in] control_name The name of the control as it
            ///        is requested by the push_control() method.
            /// @param [in] msr_name Vector of MSR names that are used
            ///        to apply the control.
            /// @param [in] field_name Vector of field names that
            ///        are written to in each corresponding MSR in
            ///        the msr_name vector.
            void register_msr_control(const std::string &control_name,
                                      const std::vector<std::string> &msr_name,
                                      const std::vector<std::string> &field_name);
            /// @brief Activate all signals and controls that have
            ///        been pushed since initialization or last call
            ///        to clear().
            virtual void activate(void);
            /// @brief Call map_field() for MSRSignals and MSRConrols
            ///        and configure MSRIO.
            virtual void activate_msr(void);

            const int m_num_cpu;
            bool m_is_active;
            IMSRIO *m_msrio;
            std::map<std::string, const IMSR *> m_name_msr_map;
            std::map<std::string, std::vector<ISignal *> > m_name_cpu_signal_map;
            std::map<std::string, std::vector<IControl *> > m_name_cpu_control_map;
            std::vector<ISignal *> m_active_signal;
            std::vector<IControl *> m_active_control;
            std::vector<uint64_t> m_msr_read_field;
            std::vector<uint64_t> m_msr_write_field;
            std::vector<int> m_msr_read_cpu_idx;
            std::vector<uint64_t> m_msr_read_offset;
            std::vector<int> m_msr_write_cpu_idx;
            std::vector<uint64_t> m_msr_write_offset;
            std::vector<uint64_t> m_msr_write_mask;
    };
}

#endif
