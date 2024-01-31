/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
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
            /// @param [in] cpu_index Index of the CPU to which the interface
            ///             write is being issued.
            /// @param [in] command Which SST interface command to issue.
            /// @param [in] subcommand Which SST interface subcommand to issue.
            /// @param [in] interface_parameter Which SST mailbox parameter to use.
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
            void set_write_dependency(uint64_t trigger_value, std::weak_ptr<geopm::Control> dependency, uint64_t dependency_write_value);

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

                uint64_t m_trigger_write_value;
                std::weak_ptr<geopm::Control> m_dependency;
                uint64_t m_dependency_write_value;
    };
}

#endif
