/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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
