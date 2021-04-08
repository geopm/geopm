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

#ifndef SSTCONTROL_HPP_INCLUDE
#define SSTCONTROL_HPP_INCLUDE

#include <memory>

#include "Control.hpp"

namespace geopm
{
    class SSTIO;

    /// This is the abstraction layer that exposes GEOPM controls from the
    /// SSTIO object.
    class SSTControl : public geopm::Control
    {
        public:
            enum ControlType
            {
                M_MBOX, // SST Mailbox interface
                M_MMIO  // SST MMIO interface
            };

            /// @brief Create an SSTIO Control.
            /// @param [in] sstio Interface through which SST interactions are handled.
            /// @param [in] control_type Which SST interface to use.
            /// @param [in] cpu_index Index of the cpu to which the interface
            ///             write is being issued.
            /// @param [in] command Which SST interface command to issue.
            /// @param [in] subcommand Which SST interface subcommand to issue.
            /// @param [in] interface_parameter Which SST mailbox paramter to use.
            /// @param [in] write_value The value to write to the interface.
            /// @param [in] begin_bit The first (least-significant) bit to
            ///             include in the write mask.
            /// @param [in] end_bit The last bit to include in the write mask.
            /// @param [in] scale The scaling factor to apply to written values.
            /// @param [in] rmw_subcommand Which subcommand to use for read
            ///             as part of read-modify-write. This is not always
            ///             the same as the write subcommand.
            /// @param [in] rmw_interface_parameter Which interface parameter to
            ///             use for read as part of read-modify-write. This is
            ///             not always the same as the write interface parameter.
            /// @param [in] rmw_read_mask Which mask to use for read as part of
            ///             read-modify-write. This is not always the same as
            ///             the write mask.
            SSTControl(std::shared_ptr<SSTIO> sstio, ControlType control_type,
                       int cpu_idx, uint32_t command, uint32_t subcommand,
                       uint32_t interface_parameter, uint32_t write_value, uint32_t begin_bit,
                       uint32_t end_bit, double scale, uint32_t rmw_subcommand,
                       uint32_t rmw_interface_parameter, uint32_t rmw_read_mask);
            virtual ~SSTControl() = default;
            void setup_batch(void) override;
            void adjust(double value) override;
            void write(double value) override;
            void save(void) override;
            void restore(void) override;
            private:
                std::shared_ptr<SSTIO> m_sstio;
                const ControlType m_control_type;
                const int m_cpu_idx;
                const uint32_t m_command;
                const uint32_t m_subcommand;
                const uint32_t m_interface_parameter;
                const uint32_t m_write_value;
                int m_adjust_idx;
                const int m_shift;
                const int m_num_bit;
                const uint64_t m_mask;
                const uint32_t m_rmw_subcommand;
                const uint32_t m_rmw_interface_parameter;
                const uint32_t m_rmw_read_mask;
                const double m_multiplier;
                uint32_t m_saved_value;
    };
}

#endif
