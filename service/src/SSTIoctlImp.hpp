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
