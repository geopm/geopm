/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef SSTIOCTL_HPP_INCLUDE
#define SSTIOCTL_HPP_INCLUDE

#include <cstdint>

#include <memory>
#include <vector>

namespace geopm
{
    /// SST Version information
    struct sst_version_s {
        uint16_t interface_version;
        uint16_t driver_version;
        uint16_t batch_command_limit;
        uint8_t is_mbox_supported;
        uint8_t is_mmio_supported;
    };

    /// A single mapping of logical CPU index to punit CPU
    struct sst_cpu_map_interface_s {
        uint32_t cpu_index;
        uint32_t punit_cpu;
    };

    /// A batch of CPU mappings
    /// @c interfaces can be variable-length. The true length must be
    /// defined in @c num_entries.
    struct sst_cpu_map_interface_batch_s {
        uint32_t num_entries;
        sst_cpu_map_interface_s interfaces[1];
    };

    /// A single MMIO operation
    struct sst_mmio_interface_s {
        uint32_t is_write;
        uint32_t cpu_index;
        uint32_t register_offset;
        uint32_t value;
    };

    /// A batch of MMIO operations
    /// @c interfaces can be variable-length. The true length must be
    /// defined in @c num_entries.
    struct sst_mmio_interface_batch_s {
        uint32_t num_entries;
        sst_mmio_interface_s interfaces[1];
    };

    /// A single mailbox operation
    struct sst_mbox_interface_s {
        uint32_t cpu_index;
        uint32_t mbox_interface_param; // Parameter to the mbox interface
        // itself
        uint32_t write_value; // Mailbox data, or input parameter for a read
        uint32_t read_value;  // Mailbox data (read-only)
        uint16_t command;
        uint16_t subcommand;
        uint32_t reserved;
    };

    /// A batch of mailbox operations
    /// @c interfaces can be variable-length. The true length must be
    /// defined in @c num_entries.
    struct sst_mbox_interface_batch_s {
        uint32_t num_entries;
        sst_mbox_interface_s interfaces[1];
    };

    /// @brief Defines functions that interact directly with SST ioctls
    class SSTIoctl
    {
        public:
            virtual ~SSTIoctl() = default;

            /// @brief Send an ioctl to the SST version interface
            /// @param [out] version SST version information.
            virtual int version(sst_version_s *version) = 0;

            /// @brief Get mappings of logical CPUs to punit CPUs
            /// @param [in,out] cpu_batch a set of CPU mappings. The maximum number
            ///                 of mappings per ioctl request is specified by the
            ///                 SST version information.
            virtual int get_cpu_id(sst_cpu_map_interface_batch_s *cpu_batch) = 0;

            /// @brief Interact with the SST mailbox. This may be for send or
            ///        receive operations.
            /// @param [in,out] mbox_batch collection of operations to perform
            ///                 in this ioctl call. The maximum count of operations
            ///                 is specified by the SST version information.
            virtual int mbox(sst_mbox_interface_batch_s *mbox_batch) = 0;

            /// @brief Interact with the SST MMIO interface. This may be for
            ///        read or write  operations.
            /// @param [in,out] mmio_batch collection of operations to perform
            ///                 in this ioctl call. The maximum count of operations
            ///                 is specified by the SST version information.
            virtual int mmio(sst_mmio_interface_batch_s *mmio_batch) = 0;

            /// @brief create an object to interact with this interface.
            /// @param path Path to the ioctl node.
            static std::shared_ptr<SSTIoctl> make_shared(const std::string &path);
    };
}
#endif
