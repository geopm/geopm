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

#ifndef MSRCONTROL_HPP_INCLUDE
#define MSRCONTROL_HPP_INCLUDE

#include <cstdint>

#include <string>
#include <memory>

namespace geopm
{
    class MSR;

    class MSRControl
    {
        public:
            MSRControl() = default;
            virtual ~MSRControl() = default;
            /// @brief Make a copy of the concrete object and call
            ///        map_field() on the new object.
            virtual std::unique_ptr<MSRControl> copy_and_remap(uint64_t *field,
                                                               uint64_t *mask) const = 0;
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
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<MSRControl> make_unique(const MSR &msr_obj,
                                                           int domain_type,
                                                           int cpu_idx,
                                                           int control_idx);
    };
}

#endif
