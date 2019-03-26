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

#ifndef MSRCONTROLIMP_HPP_INCLUDE
#define MSRCONTROLIMP_HPP_INCLUDE

#include "MSRControl.hpp"

namespace geopm
{
    class MSRControlImp : public MSRControl
    {
        public:
            /// @brief Constructor for the MSRControlImp class used when the
            ///        control is enforced with a single bit field in a
            ///        single MSR.
            /// @param [in] msr_obj Pointer to the MSR object
            ///        describing the MSR that contains the control.
            /// @param [in] cpu_idx The logical Linux CPU index to
            ///        write the MSR.
            /// @param [in] control_idx The index of the control within
            ///        the MSR that the class represents.
            MSRControlImp(const MSR &msr_obj,
                          int domain_type,
                          int cpu_idx,
                          int control_idx);
            std::unique_ptr<MSRControl> copy_and_remap(uint64_t *field,
                                                       uint64_t *mask) const override;
            virtual ~MSRControlImp();
            virtual std::string name(void) const override;
            int domain_type(void) const override;
            int cpu_idx(void) const override;
            void adjust(double setting) override;
            uint64_t offset(void) const override;
            uint64_t mask(void) const override;
            void map_field(uint64_t *field, uint64_t *mask) override;
        private:
            // Copying is disallowed except through copy_and_remap() method
            MSRControlImp(const MSRControlImp &other);
            MSRControlImp &operator=(const MSRControlImp &other) = default;

            const std::string m_name;
            const MSR &m_msr_obj;
            const int m_domain_type;
            const int m_cpu_idx;
            const int m_control_idx;
            uint64_t *m_field_ptr;
            uint64_t *m_mask_ptr;
            bool m_is_field_mapped;
    };
}

#endif
