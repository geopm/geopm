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

#ifndef MSRIO_HPP_INCLUDE
#define MSRIO_HPP_INCLUDE

#include <cstdint>
#include <vector>
#include <memory>

namespace geopm
{
    class MSRIO
    {
        public:
            MSRIO() = default;
            virtual ~MSRIO() = default;
            /// @brief Read from a single MSR on a CPU.
            /// @brief [in] cpu_idx logical Linux CPU index to read
            ///        from.
            /// @param [in] offset The MSR offset to read from.
            /// @return The raw encoded MSR value read.
            virtual uint64_t read_msr(int cpu_idx,
                                      uint64_t offset) = 0;
            /// @brief Write to a single MSR on a CPU.
            /// @param [in] cpu_idx logical Linux CPU index to write
            ///        to.
            /// @param [in] offset The MSR offset to write to.
            /// @param [in] raw_value The raw encoded MSR value to
            ///        write, only bits where the write_mask is set
            ///        will be written, other bits in the MSR will be
            ///        unmodified.
            /// @param [in] write_mask The mask determines the bits of
            ///        the MSR that will be modified.
            virtual void write_msr(int cpu_idx,
                                   uint64_t offset,
                                   uint64_t raw_value,
                                   uint64_t write_mask) = 0;
            /// @brief initialize internal data structures to batch
            ///        read/write from MSRs.
            /// @param [in] read_cpu_idx A vector of logical Linux CPU
            ///        indices to read from when read_batch() method
            ///        is called.
            /// @param [in] read_offset A vector of the MSR offsets to
            ///        be read from when the read_batch() method is
            ///        called.
            /// @param [in] write_cpu_idx A vector of logical Linux
            ///        CPU indices to write to when the write_batch()
            ///        method is called.
            /// @param [in] write_offset A vector of the MSR offset to
            ///        be written to when the write_batch() method is
            ///        called.
            /// @param [in] write_mask A vector of write masks that
            ///        will determine the bits of the MSRs to be
            ///        modified when the write_batch() method is s
            ///        called.
            virtual void config_batch(const std::vector<int> &read_cpu_idx,
                                      const std::vector<uint64_t> &read_offset,
                                      const std::vector<int> &write_cpu_idx,
                                      const std::vector<uint64_t> &write_offset,
                                      const std::vector<uint64_t> &write_mask) = 0;
            /// @brief Batch read a set of MSRs configured by a
            ///        previous call to the batch_config() method.
            /// @param [out] raw_value The raw encoded MSR values to
            ///        be read.
            virtual void read_batch(std::vector<uint64_t> &raw_value) = 0;
            /// @brief Batch write a set of MSRs configured by a
            ///        previous call to the batch_config() method.
            /// @param [in] raw_value The raw encoded MSR values to be
            ///        written.
            virtual void write_batch(const std::vector<uint64_t> &raw_value) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<MSRIO> make_unique(void);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<MSRIO> make_shared(void);
    };
}

#endif
