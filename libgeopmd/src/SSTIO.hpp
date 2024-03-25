/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SSTIO_HPP_INCLUDE
#define SSTIO_HPP_INCLUDE

#include <cstdint>

#include <vector>
#include <memory>

namespace geopm
{
    class SSTIO
    {
        public:
            /// @brief Interact with the mailbox on commands that are expected to return data
            /// @param [in] cpu_index Index of the CPU to which the mailbox
            ///             read is being issued
            /// @param [in] command Which SST mailbox command to issue
            /// @param [in] subcommand Which SST mailbox subcommand to issue
            /// @param [in] subcommand_arg Which argument to use for the SST
            ///             mailbox subcommand
            virtual int add_mbox_read(uint32_t cpu_index, uint16_t command,
                                      uint16_t subcommand, uint32_t subcommand_arg) = 0;

            /// @brief Interact with the mailbox on commands that are not expected to return data
            /// @param [in] cpu_index Index of the CPU to which the mailbox
            ///             read is being issued
            /// @param [in] command Which SST mailbox command to issue
            /// @param [in] subcommand Which SST mailbox subcommand to issue
            /// @param [in] interface_parameter Which SST mailbox parameter to use
            /// @param [in] read_subcommand Which SST mailbox subcommand to issue
            ///             when reading the current value prior to a write
            /// @param [in] read_interface_parameter Which SST mailbox parameter to use
            ///             when reading the current value prior to a write
            /// @param [in] read_mask The mask to apply to values read from the
            ///             mailbox prior to a write
            virtual int add_mbox_write(uint32_t cpu_index, uint16_t command,
                                       uint16_t subcommand, uint32_t interface_parameter,
                                       uint16_t read_subcommand,
                                       uint32_t read_interface_parameter,
                                       uint32_t read_mask) = 0;

            /// @brief Interact with the mmio interface on commands that are expected to return data
            /// @param [in] cpu_index Index of the CPU to which the MMIO
            ///             read is being issued
            /// @param [in] register_offset Which SST MMIO register offset to use
            virtual int add_mmio_read(uint32_t cpu_index, uint16_t register_offset) = 0;

            /// @brief Interact with the mmio interface on commands that are not expected to return data
            /// @param [in] cpu_index Index of the CPU to which the MMIO
            ///             write is being issued
            /// @param [in] register_offset Which SST MMIO register offset to use
            /// @param [in] register_value Which SST MMIO register value to set
            ///             for the write.
            /// @param [in] read_mask The mask to apply to values read from the
            ///             register prior to a write
            virtual int add_mmio_write(uint32_t cpu_index, uint16_t register_offset,
                                       uint32_t register_value,
                                       uint32_t read_mask) = 0;

            /// @brief Issue a batch read
            virtual void read_batch(void) = 0;

            /// @brief Sample a value from the most recent batch read
            /// @param [in] batch_idx An index returned from an add_*_read function
            ///             prior to calling read_batch().
            virtual uint64_t sample(int batch_idx) const = 0;

            /// @brief Issue a batch write
            virtual void write_batch(void) = 0;

            /// @brief Immediately query the SST mailbox for a read operation.
            /// @param [in] cpu_index Index of the CPU to which the mailbox
            ///             read is being issued
            /// @param [in] command Which SST mailbox command to issue
            /// @param [in] subcommand Which SST mailbox subcommand to issue
            /// @param [in] subcommand_arg Which argument to use for the SST
            ///             mailbox subcommand
            virtual uint32_t read_mbox_once(uint32_t cpu_index, uint16_t command,
                                            uint16_t subcommand, uint32_t subcommand_arg) = 0;

            /// @brief Immediately query the SST mailbox for a write operation.
            /// @param [in] cpu_index Index of the CPU to which the mailbox
            ///             read is being issued
            /// @param [in] command Which SST mailbox command to issue
            /// @param [in] subcommand Which SST mailbox subcommand to issue
            /// @param [in] interface_parameter Which SST mailbox parameter to use
            /// @param [in] read_subcommand Which SST mailbox subcommand to issue
            ///             when reading the current value prior to a write
            /// @param [in] read_interface_parameter Which SST mailbox parameter to use
            ///             when reading the current value prior to a write
            /// @param [in] read_mask The mask to apply to values read from the
            ///             mailbox prior to a write
            /// @param [in] write_value The value to write
            /// @param [in] write_mask The mask to apply to the written value
            virtual void write_mbox_once(uint32_t cpu_index, uint16_t command,
                                         uint16_t subcommand,
                                         uint32_t interface_parameter,
                                         uint16_t read_subcommand,
                                         uint32_t read_interface_parameter,
                                         uint32_t read_mask, uint64_t write_value,
                                         uint64_t write_mask) = 0;

            /// @brief Immediately read a value from the SST MMIO interface.
            /// @param [in] cpu_index Index of the CPU to which the MMIO
            ///             read is being issued
            /// @param [in] register_offset Which SST MMIO register offset to use
            virtual uint32_t read_mmio_once(uint32_t cpu_index, uint16_t register_offset) = 0;

            /// @brief Immediately write a value to the SST MMIO interface.
            /// @param [in] cpu_index Index of the CPU to which the MMIO
            ///             write is being issued
            /// @param [in] register_offset Which SST MMIO register offset to use
            /// @param [in] register_value Which SST MMIO register value to set
            ///             for the write.
            /// @param [in] read_mask The mask to apply to values read from the
            ///             register prior to a write
            /// @param [in] write_value The value to write
            /// @param [in] write_mask The mask to apply to the written value
            virtual void write_mmio_once(uint32_t cpu_index, uint16_t register_offset,
                                         uint32_t register_value,
                                         uint32_t read_mask, uint64_t write_value,
                                         uint64_t write_mask) = 0;

            /// @brief Adjust a value for the next batch write
            /// @param [in] batch_idx An index returned from an add_*_write function
            /// @param [in] write_value The value to write in the next batch_write()
            /// @param [in] write_mask The mask to apply when writing this value
            virtual void adjust(int batch_idx, uint64_t write_value, uint64_t write_mask) = 0;

            /// @brief Get the punit index associated with a CPU index
            /// @param [in] cpu_index Index of the CPU
            virtual uint32_t get_punit_from_cpu(uint32_t cpu_index) = 0;

            /// @brief Create an SSTIO object
            /// @param [in] max_cpus The number of CPUs to attempt to map
            ///             to punit cores.
            static std::shared_ptr<SSTIO> make_shared(uint32_t max_cpus);
    };

}

#endif
