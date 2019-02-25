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

#ifndef MSRSIGNAL_HPP_INCLUDE
#define MSRSIGNAL_HPP_INCLUDE

#include <stdint.h>

#include <string>
#include <memory>

namespace geopm
{
    class IMSR;

    class IMSRSignal
    {
        public:
            IMSRSignal() = default;
            virtual ~IMSRSignal() = default;
            /// @brief Make a copy of the concrete object.
            ///        The map_field() method should be called on
            ///        the new object.
            virtual std::unique_ptr<IMSRSignal> clone(void) const = 0;
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
            virtual ~MSRSignal() = default;
            std::unique_ptr<IMSRSignal> clone(void) const override;
            std::string name(void) const override;
            int domain_type(void) const override;
            int cpu_idx(void) const override;
            double sample(void) override;
            uint64_t offset(void) const override;
            void map_field(const uint64_t *field) override;
        private:
            // Copying is disallowed except through clone() method
            MSRSignal(const MSRSignal &other);
            MSRSignal &operator=(const MSRSignal &other) = delete;

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
}

#endif
