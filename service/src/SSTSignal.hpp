/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            /// @param [in] cpu_index Index of the CPU to which the mailbox
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
