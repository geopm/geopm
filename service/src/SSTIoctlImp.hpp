/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef SSTIOCTLIMP_HPP_INCLUDE
#define SSTIOCTLIMP_HPP_INCLUDE

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#include "SSTIoctl.hpp"

namespace geopm
{

    class SSTIoctlImp final : public SSTIoctl
    {
        public:
            /// @brief create an object to interact with this interface.
            /// @param path Path to the ioctl node.
            SSTIoctlImp(const std::string &path);
            SSTIoctlImp(const SSTIoctlImp &other) = delete;
            SSTIoctlImp &operator=(const SSTIoctlImp &other) = delete;
            ~SSTIoctlImp();

            /// @brief Send an ioctl to the SST version interface
            /// @param [out] version SST version information.
            int version(sst_version_s *version) override;

            /// @brief Get mappings of logical CPUs to punit CPUs
            /// @param [in,out] cpu_batch a set of CPU mappings. The maximum number
            ///                 of mappings per ioctl request is specified by the
            ///                 SST version information.
            int get_cpu_id(sst_cpu_map_interface_batch_s *cpu_batch) override;

            /// @brief Interact with the SST mailbox. This may be for send or
            ///        receive operations.
            /// @param [in,out] mbox_batch collection of operations to perform
            ///                 in this ioctl call. The maximum count of operations
            ///                 is specified by the SST version information.
            int mbox(sst_mbox_interface_batch_s *mbox_batch) override;

            /// @brief Interact with the SST MMIO interface. This may be for
            ///        read or write  operations.
            /// @param [in,out] mmio_batch collection of operations to perform
            ///                 in this ioctl call. The maximum count of operations
            ///                 is specified by the SST version information.
            int mmio(sst_mmio_interface_batch_s *mmio_batch) override;
        private:
            const std::string m_path;
            const int m_fd;
    };
}
#endif
