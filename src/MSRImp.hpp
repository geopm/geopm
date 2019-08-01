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

#ifndef MSRIMP_HPP_INCLUDE
#define MSRIMP_HPP_INCLUDE

#include <vector>

#include "MSR.hpp"

namespace geopm
{
    class MSREncode;

    class MSRImp : public MSR
    {
        public:
            /// @brief Constructor for the MSRImp class for fixed MSRs.
            /// @param [in] msr_name The name of the MSR.
            /// @param [in] offset The byte offset of the MSR.
            /// @param [in] signal Vector of signal name and encode
            ///        struct pairs describing all signals embedded in
            ///        the MSR.
            /// @param [in] control Vector of control name and encode
            /// struct pairs describing all controls embedded in the
            /// MSR.
            MSRImp(const std::string &msr_name,
                   uint64_t offset,
                   const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &signal,
                   const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &control);
            virtual ~MSRImp();
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
            int units(int signal_idx) const override;
        private:
            void init(const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &signal,
                      const std::vector<std::pair<std::string, struct MSR::m_encode_s> > &control);
            std::string m_name;
            uint64_t m_offset;
            std::vector<MSREncode *> m_signal_encode;
            std::vector<MSREncode *> m_control_encode;
            std::map<std::string, int> m_signal_map;
            std::map<std::string, int> m_control_map;
            int m_domain_type;
            const std::vector<const MSR *> m_prog_msr;
            const std::vector<std::string> m_prog_field_name;
            const std::vector<double> m_prog_value;

    };
}

#endif
