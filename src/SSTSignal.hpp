/*
 * Copyright (c) 2015 - 2021, Intel Corporation
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

#ifndef SSTSIGNAL_HPP_INCLUDE
#define SSTSIGNAL_HPP_INCLUDE

#include <memory>

#include "Signal.hpp"

namespace geopm
{
    class SSTIO;

    /// This is the abstraction layer that exposes GEOPM signals from the
    /// SSTIO object.
    class SSTSignal : public geopm::Signal
    {
        public:
            enum SignalType
            {
                M_MBOX, // SST Mailbox interface
                M_MMIO  // SST MMIO interface
            };

            /// @brief Create an SSTIO Signal.
            /// @param [in] sstio Interface through which SST interactions are handled.
            /// @param [in] signal_type Which SST interface to use.
            /// @param [in] cpu_index Index of the cpu to which the mailbox
            ///             read is being issued.
            /// @param [in] command Which SST interface command to issue.
            /// @param [in] subcommand Which SST interface subcommand to issue
            /// @param [in] subcommand_arg Which SST interface subcommand
            ///             argument to use.
            /// @param [in] interface_parameter Which SST interface parameter
            ///             to use.
            SSTSignal(std::shared_ptr<geopm::SSTIO> sstio,
                      SignalType signal_type,
                      int cpu_idx,
                      uint16_t command,
                      uint16_t subcommand,
                      uint32_t subcommand_arg,
                      uint32_t interface_parameter);

            virtual ~SSTSignal() = default;

            void setup_batch(void) override;
            double sample(void) override;
            double read(void) const override;
        private:
            std::shared_ptr<geopm::SSTIO> m_sstio;
            SignalType m_signal_type;
            const int m_cpu_idx;
            const uint16_t m_command;
            const uint16_t m_subcommand;
            const uint32_t m_subcommand_arg;

            int m_batch_idx;
    };

}

#endif
